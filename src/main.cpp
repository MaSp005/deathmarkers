#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/platform/platform.hpp>
#include <vector>
#include <string>
#include <stdlib.h>
#include "shared.hpp"
#include "submitter.hpp"
#include "lib/sha1.hpp"

using namespace geode::prelude;
using namespace dm;

enum DMEvent {
	LEVEL_LOAD,
	DEATH,
	PAUSE,
	RESET,
	TOGGLE_PRACTICE,
	RESUME,
};

enum DMDrawScope {
	// Nothing should be drawn
	NONE,
	// Markers on screen should be drawn
	LOCAL,
	// All markers should be drawn
	GLOBAL
};

#include <Geode/modify/PlayLayer.hpp>
class $modify(DMPlayLayer, PlayLayer) {

	struct Fields {
		// WebRequest response listener for death listing
		async::TaskHolder<WebResponse> m_listener;

		// Node holding all marker nodes
		CCNode* m_dmNode = CCNode::create();
		// Node on the progress bar holding its chart
		CCDrawNode* m_chartNode = nullptr;

		// Current visibility of markers
		DMDrawScope m_drawn = NONE;
		// false if the level or settings will never result in markers being shown
		bool m_willEverDraw = true;

		// List of deaths
		vector<unique_ptr<DeathLocationMin>> m_deaths;
		// Iterator to an entry in m_deaths pointing to the death that was last added
		vector<unique_ptr<DeathLocationMin>>::iterator m_latest;
		// List of pending submissions, used to send on level exit
		vector<DeathLocationOut> m_submissions;

		// True if m_chartNode is attached, false if it is yet to
		bool m_chartAttached = false;
		// Whether m_deaths has been populated with online deaths
		bool m_fetched = false;
		// Holds player properties for submission
		struct playerData m_playerProps;
		// Holds level properties for submission
		struct playingLevel m_levelProps;

		// Whether user settings signify local deaths should be used for this level
		bool m_useLocal = false;
		// Whether user settings signify only normal mode deaths should be shown
		bool m_normalOnly = false;
	};

	bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {

		if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

		// Prepare UI
		this->m_fields->m_dmNode->setID("markers"_spr);
		this->m_fields->m_dmNode->setZOrder(2 << 28); // everyone using 99999 smh
		this->m_objectLayer->addChild(this->m_fields->m_dmNode);

		// refetch on level start in case it changed (player changed account)
		// CONSIDER: Use GJAccountManager instead to ban unstable non-account users
		this->m_fields->m_playerProps.userid =
			GameManager::get()->m_playerUserID.value();
		this->m_fields->m_playerProps.username =
			GameManager::get()->m_playerName;

		auto mod = Mod::get();

		// save level data
		this->m_fields->m_levelProps.levelId = level->m_levelID;
		this->m_fields->m_levelProps.levelversion = level->m_levelVersion;
		this->m_fields->m_levelProps.platformer = level->isPlatformer();
		this->m_fields->m_levelProps.practice = this->m_isPracticeMode;
		this->m_fields->m_levelProps.testmode = this->m_isTestMode;

		std::string storeLocalStr = mod->getSettingValue<std::string>("store-local-2");
		this->m_fields->m_useLocal = storeLocalStr == "Always" ? true : storeLocalStr == "Never" ? false :
			this->m_level->m_stars >= 10;
		this->m_fields->m_normalOnly = mod->getSettingValue<bool>("normal-only");
		GhostLocation::shouldUse = mod->getSettingValue<bool>("use-ghost-cube");

		log::debug("{} {} {}", storeLocalStr, this->m_level->m_stars, this->m_fields->m_useLocal);

		// Don't even continue to list if we're not going to show them anyway
		this->m_fields->m_willEverDraw =
			dm::willEverDraw(this->m_fields->m_levelProps);
		if (!this->m_fields->m_willEverDraw) {
			this->m_fields->m_fetched = true;
			return true;
		}

		this->m_fields->m_deaths.clear();
		this->m_fields->m_submissions.clear();

		this->fetch(
			[this](bool success) {
				if (!success)
					return Notification::create("DeathMarkers could not load")->show();
				this->checkDraw(LEVEL_LOAD);
			}
		);

		this->schedule(schedule_selector(DMPlayLayer::updateMarkers), 0);

		return true;

	}

	// Sometimes this method creates the progress bar delayed
	void setupHasCompleted() {

		PlayLayer::setupHasCompleted();
		if (this->m_fields->m_drawn) this->renderHistogram();

	}

	void fetch(std::function<void(bool)> cb) {

		if (this->m_fields->m_fetched) return cb(true);

		log::info("Listing Deaths...");

		if (this->m_fields->m_useLocal) {
			// Normally, when fetching we append, but this will not fail
			// so we can safely clear and overwrite here.
			this->m_fields->m_deaths.clear();
			this->m_fields->m_deaths = getLocalDeaths(
				this->m_fields->m_levelProps.levelId,
				!this->m_fields->m_levelProps.platformer
			);
			log::debug("Finished parsing local saves.");
			this->m_fields->m_fetched = true;
			return cb(true);
		}

		// Build the HTTP Request
		web::WebRequest req = web::WebRequest();

		req.param("levelid", this->m_fields->m_levelProps.levelId);
		req.param("platformer", this->m_fields->m_levelProps.platformer ? "true" : "false");
		req.param("practice", this->m_fields->m_normalOnly ? "false" : "true");
		req.param("response", "bin");
		req.userAgent(HTTP_AGENT);
		req.timeout(HTTP_TIMEOUT);

		this->m_fields->m_listener.spawn(
			req.get(dm::makeRequestURL("list")),
			
			// Parse result JSON and add all as DeathLocationMin instances to playingLevel.deaths
			[this, cb](WebResponse res) {
				if (!res.ok()) {
					log::error("Listing Deaths failed: {}",
									res.string().unwrapOr("Body could not be read."));
					cb(false);
				} else {
					log::debug("Received death list.");
					parseBinDeathList(res, &this->m_fields->m_deaths, !this->m_fields->m_levelProps.platformer);
					sort(
						this->m_fields->m_deaths.begin(),
						this->m_fields->m_deaths.end(),
						LocationComparerPtr{}
					);
					log::debug("Finished parsing.");
					this->m_fields->m_fetched = true;

					cb(true);
				}
			}
		);

	}

	void resetLevel() {

		PlayLayer::resetLevel();
		this->checkDraw(RESET);

	}

	void pauseGame(bool p0) {

		PlayLayer::pauseGame(p0);
		this->checkDraw(PAUSE);

	}

	void resume() {

		PlayLayer::resume();
		this->checkDraw(RESUME);

	}

	void levelComplete() {

		PlayLayer::levelComplete();
		if (this->m_fields->m_levelProps.platformer) return;

		auto deathLoc = DeathLocationOut(this->getPosition());
		deathLoc.percentage = 101;
		this->m_fields->m_submissions.push_back(deathLoc);

	}

	void togglePracticeMode(bool toggle) {

		PlayLayer::togglePracticeMode(toggle);
		this->m_fields->m_levelProps.practice = toggle;

		this->checkDraw(TOGGLE_PRACTICE);

	}

	void onQuit() {

		PlayLayer::onQuit();
		this->submitDeaths();

		if (this->m_fields->m_useLocal && this->m_fields->m_willEverDraw) {
			storeLocalDeaths(
				this->m_fields->m_levelProps.levelId,
				this->m_fields->m_deaths,
				!this->m_fields->m_levelProps.platformer
			);
		}

		this->m_fields->m_deaths.clear();

	}

	void renderMarkers(vector<unique_ptr<DeathLocationMin>>::iterator begin,
		vector<unique_ptr<DeathLocationMin>>::iterator end, bool animate) {

		if (end == this->m_fields->m_deaths.end()) {
			if (begin == end) return; // Nothing to draw
			// Prevent crash
			--end;
		}

		double fadeTime = Mod::get()->getSettingValue<float>("fade-time") / 2;
		for (auto deathLoc = begin; deathLoc <= end; ++deathLoc) {
			CCNode* node;
			if (animate) node = (*deathLoc)->createAnimatedNode(
				deathLoc == this->m_fields->m_latest,
				(static_cast<double>(rand()) / RAND_MAX) * fadeTime,
				fadeTime
			);
			else node = (*deathLoc)->createNode(deathLoc == this->m_fields->m_latest);
			this->m_fields->m_dmNode->addChild(node);
		}
		updateMarkers(0.0f);

	}

	void updateMarkers(float) {

		auto sceneRotation = this->m_gameState.m_cameraAngle;
		float inverseScale = Mod::get()->getSettingValue<float>("marker-scale") /
			this->m_objectLayer->getScale();
		if (inverseScale < 0) inverseScale *= -1;

		auto children = this->m_fields->m_dmNode->getChildren();
		for (int i = 0; i < this->m_fields->m_dmNode->getChildrenCount(); i++) {
			auto child = static_cast<CCNode*>(children->objectAtIndex(i));
			bool isCurrent = child->getZOrder() == CURRENT_ZORDER;

			if (!cast::typeinfo_cast<SimplePlayer*>(child)) {
				// Only for regular markers
				child->setScale((isCurrent ? 1.5f : 1.0f) * inverseScale);
				child->setRotation(-sceneRotation);
			}
		}

	}

	void renderHistogram() {

		int histHeight = Mod::get()->getSettingValue<int>("prog-bar-hist-height");

		// Only Draw Histogram if requested and applicable
		if (histHeight == 0 || this->m_fields->m_levelProps.platformer) return;

		int hist[101] = { 0 };

		for (auto& deathLoc : this->m_fields->m_deaths) {
			if (deathLoc->percentage >= 0 && deathLoc->percentage < 101)
				hist[deathLoc->percentage]++;
		}

		if (!this->m_fields->m_chartAttached) {
			auto progBarNode = this->m_progressBar;
			if (!progBarNode) return;

			this->m_fields->m_chartNode = CCDrawNode::create();
			this->m_fields->m_chartNode->setID("chart"_spr);
			this->m_fields->m_chartNode->setZOrder(-2);
			this->m_fields->m_chartNode->setPosition(2, 4);
			this->m_fields->m_chartNode->setContentWidth(
				progBarNode->getContentWidth() - 4
			);
			progBarNode->addChild(this->m_fields->m_chartNode);
			this->m_fields->m_chartAttached = true;
		}

		int maximum = hist[0];
		float width = this->m_fields->m_chartNode->getContentWidth() / 100;

		for (int i = 1; i <= 100; i++) {
			if (hist[i] > maximum)
				maximum = hist[i];
		}

		this->m_fields->m_chartNode->clear();
		for (int i = 0; i <= 100; i++) {
			if (hist[i] == 0) continue;

			float distr = static_cast<float>(hist[i]) / maximum;
			auto rect = CCRect(width * i, 0, width, -(distr * histHeight));
			auto pos1 = CCPoint(width * i, 0);
			auto pos2 = CCPoint(width * i + width, -(distr * histHeight));
			ccColor4F color{};
			color.r = 1 - ((1 - distr) * (1 - distr));
			color.g = 1 - (distr * distr);
			color.b = 0;
			color.a = 1;

			this->m_fields->m_chartNode->drawRect(pos1, pos2, color, 0.0f, color);
		}

	}

	void clearMarkers() {

		m_fields->m_dmNode->removeAllChildrenWithCleanup(true);
		m_fields->m_dmNode->cleanup();

		if (!this->m_fields->m_chartAttached) return;
		this->m_fields->m_chartNode->clear();
		this->m_fields->m_chartNode->cleanup();

	}

	void findDeathRangeInFrame(
		vector<unique_ptr<DeathLocationMin>>::iterator& begin,
		vector<unique_ptr<DeathLocationMin>>::iterator& end,
		float lenience = 0.0f) {

		// For all this jargon, see the "Screen Limit" slide in docs/doc.dio
		auto winSize = CCDirector::sharedDirector()->getWinSize();
		auto halfWinWidth = winSize.width / 2;
		float winDiagonal =
			(sqrt(winSize.width * winSize.width + winSize.height * winSize.height)
			/ this->m_objectLayer->getScale() + 70) / 2;
		// log::debug("{} {}", halfWinWidth, winDiagonal);

		begin = binarySearchNearestXPosOnScreen(begin, end, this->m_objectLayer,
			halfWinWidth - winDiagonal + min(lenience, 0.0f), false);
		end = binarySearchNearestXPosOnScreen(begin, end, this->m_objectLayer,
			halfWinWidth + winDiagonal + max(lenience, 0.0f), true);

	}

	void renderMarkersInFrame(bool animate) {

		auto begin = this->m_fields->m_deaths.begin();
		auto end = this->m_fields->m_deaths.end();

		findDeathRangeInFrame(begin, end);

		renderMarkers(begin, end, animate);

	}

	// Provides a central, consistent way to handle drawing state
	DMDrawScope shouldDraw() {

		if (!this->m_fields->m_willEverDraw) return NONE;

		auto mod = Mod::get();
		auto pauseSetting = mod->getSettingValue<bool>("show-in-pause");
		if (pauseSetting && this->m_isPaused) return LOCAL;

		bool isPractice = this->m_fields->m_levelProps.practice ||
			this->m_fields->m_levelProps.testmode;
		auto settName = isPractice ? "condition-practice" : "condition-normal";
		// Relevant condition setting for current mode
		auto condSetting = mod->getSettingValue<std::string>(settName);

		if (condSetting == "Always") return GLOBAL;
		if (condSetting == "Never") return NONE;
		// Remainder: On Death
		if (!this->m_player1->m_isDead && !this->m_player2->m_isDead) return NONE;

		if (!mod->getSettingValue<bool>("newbest-only")) return LOCAL;
		if (this->m_fields->m_levelProps.platformer) return LOCAL;

		int best = isPractice ?
			this->m_level->m_practicePercent :
			this->m_level->m_normalPercent.value();
		if (best == 100 || getCurrentPercentInt() >= best) return LOCAL;
		return NONE;

	}

	void checkDraw(DMEvent event) {
		DMDrawScope should = this->shouldDraw();

		if (!this->m_fields->m_fetched) {
			log::debug("Deaths not fetched, deferring...");
			return;
		}

		if (should != this->m_fields->m_drawn) {
			// log::debug("Toggling...");

			switch (should) {
				case NONE:
					clearMarkers();
					break;
				case LOCAL:
					renderHistogram();
					if (this->m_fields->m_drawn == NONE) {
						renderMarkersInFrame(event != PAUSE);
						// iterator equivalent of nullptr
						this->m_fields->m_latest = this->m_fields->m_deaths.end();
					} else
						// Markers are already globally rendered, so do not rerender
						// Override `should` to prevent rerendering when switching to GLOBAL
						should = GLOBAL;
					break;
				case GLOBAL:
					renderHistogram();
					if (this->m_fields->m_drawn == LOCAL) clearMarkers();
					renderMarkers(
						this->m_fields->m_deaths.begin(),
						this->m_fields->m_deaths.end(),
						true
					);
					break;
			}
			this->m_fields->m_drawn = should;
		} else if (event == DEATH && should) {
			// = markers are not redrawn, but new one should appear
			if (this->m_fields->m_latest == this->m_fields->m_deaths.end()) return;

			double fadeTime = Mod::get()->getSettingValue<float>("fade-time") / 2;
			auto node = (*this->m_fields->m_latest)->createAnimatedNode(
				true, 0, fadeTime
			);
			this->m_fields->m_dmNode->addChild(node);
			updateMarkers(0.0f);

			this->m_fields->m_latest = this->m_fields->m_deaths.end();
			renderHistogram();
		} else if (event == RESET && should) {
			// = markers are not redrawn, but last one should shrink

			// reset all nodes to regular z-order
			auto children = this->m_fields->m_dmNode->getChildren();
			for (int i = 0; i < this->m_fields->m_dmNode->getChildrenCount(); i++) {
				static_cast<CCNode*>(children->objectAtIndex(i))
					->setZOrder(OTHER_ZORDER);
			}
			updateMarkers(0.0f);
		}
	}

	void submitDeaths() {

		if (!dm::shouldSubmit(this->m_fields->m_levelProps,
			this->m_fields->m_playerProps)) return;

		purgeSpam(this->m_fields->m_submissions);
		if (this->m_fields->m_submissions.size() == 0) return;

		auto mod = Mod::get();

		// Build the HTTP Request
		auto myjson = matjson::Value();
		myjson.set("levelid", matjson::Value(
			this->m_level->m_levelID.value()
		));
		myjson.set("levelversion", matjson::Value(
			this->m_level->m_levelVersion
		));
		myjson.set("format", matjson::Value(FORMAT_VERSION));

		// Create Userident
		std::string source = fmt::format("{}_{}_{}",
			this->m_fields->m_playerProps.username,
			this->m_fields->m_playerProps.userid,
			this->m_level->m_levelID.value()
		);
		SHA1 sha1{};
		sha1.update(source);
		std::string hashed = sha1.final();
		myjson.set("userident", matjson::Value(hashed));

		// Create deaths array
		auto deathList = matjson::Value(vector<matjson::Value>());
		for (auto i : this->m_fields->m_submissions) {
			auto obj = matjson::Value();
			i.addToJSON(&obj);
			deathList.push(obj);
		}
		myjson.set("deaths", deathList);

		web::WebRequest req = web::WebRequest();
		req.param("levelid", this->m_level->m_levelID.value());
		req.bodyJSON(myjson);

		req.userAgent(HTTP_AGENT);

		req.header("Content-Type", "application/json");
		req.header("Accept", "application/json");

		log::debug("Posting {} Deaths...", this->m_fields->m_submissions.size());
		auto submitter = new Submitter(req);
		submitter->submit();
		// deletes itself when done

	}

};

#include <Geode/modify/PlayerObject.hpp>
class $modify(DMPlayerObject, PlayerObject) {

	void playerDestroyed(bool secondPlr) {

		// Forward to original, we dont want noclip in here
		PlayerObject::playerDestroyed(secondPlr);
		if (secondPlr) return;

		auto playLayer = static_cast<DMPlayLayer*>(
			GameManager::get()->getPlayLayer()
		);
		if (!playLayer) return;

		// Check if PlayerObject is the PRIMARY one and not from Globed
		if (!(
			(!m_gameLayer->m_player1 || m_gameLayer->m_player1 == this) ||
			(!m_gameLayer->m_player2 || m_gameLayer->m_player2 == this)
		)) return;

		// Populate percentage as current time or progress percentage
		int percent = playLayer->m_fields->m_levelProps.platformer ?
			static_cast<int>(playLayer->m_attemptTime) :
			playLayer->getCurrentPercentInt();
		auto deathLoc = std::make_unique<DeathLocationOut>
			(DeathLocationOut{this->getPosition()});
		deathLoc->percentage = percent;
		// deathLoc->coin1 = ...; // This stuff is complicated... prolly gonna pr Weebifying/coins-in-pause-menu-geode to make it api public and depend on it here or sm
		// deathLoc->coin2 = ...;
		// deathLoc->coin3 = ...;
		// deathLoc->itemdata = ...; // where the hell are the counters

		bool isPractice = playLayer->m_fields->m_levelProps.practice ||
			playLayer->m_fields->m_levelProps.testmode;
		deathLoc->practice = isPractice;

		playLayer->m_fields->m_submissions.push_back(*deathLoc);

		if (!playLayer->m_fields->m_normalOnly || !isPractice) {
			auto nearest = binarySearchNearestXPos(
				playLayer->m_fields->m_deaths.begin(),
				playLayer->m_fields->m_deaths.end(),
				deathLoc->pos.x, true
			);

			unique_ptr<DeathLocationMin> toShow;
			if (Mod::get()->getSettingValue<bool>("use-ghost-cube") &&
				playLayer->m_fields->m_useLocal
			) {
				auto showGhost = std::make_unique<GhostLocation>(GhostLocation(this));
				showGhost->percentage = percent;
				toShow = std::move(showGhost);
			} else
				toShow = std::move(deathLoc);

			playLayer->m_fields->m_latest =
				playLayer->m_fields->m_deaths.insert(nearest, std::move(toShow));
		}
		playLayer->checkDraw(DEATH);

	}

};

#include <Geode/modify/PauseLayer.hpp>
class $modify(DMPauseLayer, PauseLayer) {

	struct Fields {
		geode::comm::ListenerHandle m_listener;
	};

  void customSetup() override {

    PauseLayer::customSetup();

		if (auto leftMenu = this->getChildByID("left-button-menu")) {
			auto sprite = CircleButtonSprite::createWithSprite(
				"death-marker.png"_spr,
				0.81f, CircleBaseColor::Green,
				CircleBaseSize::MediumAlt
			);
			sprite->setScale(0.6f);

			auto button = CCMenuItemExt::createSpriteExtra(
				sprite, [this](CCNode*){
					auto popup = geode::openSettingsPopup(Mod::get(), true);

					this->m_fields->m_listener = geode::Popup::CloseEvent(PopupBypass::listenForCloseOn(popup)).listen(
						[](){
							log::debug("Hello yes i am here"); // TODO remove
							auto mod = Mod::get();

							auto playLayer = static_cast<DMPlayLayer*>(PlayLayer::get());
							if (!playLayer->m_fields->m_willEverDraw) return;

							auto storeLocalStr = mod->getSettingValue<std::string>("store-local-2");
							auto useLocal = storeLocalStr == "Always" ? true : storeLocalStr == "Never" ? false :
								playLayer->m_level->m_stars >= 10;
							auto normalOnly = mod->getSettingValue<bool>("normal-only");
							GhostLocation::shouldUse = mod->getSettingValue<bool>("use-ghost-cube");

							playLayer->checkDraw(PAUSE);

							bool useLocalChanged = playLayer->m_fields->m_useLocal != useLocal;
							bool normalOnlyChanged = playLayer->m_fields->m_normalOnly != normalOnly;
							if (!useLocalChanged && !normalOnlyChanged) return;

							std::string message = "<co>";
							if (useLocalChanged) message += "\"Use Local Deats\"";
							if (useLocalChanged && normalOnlyChanged) message += " and ";
							if (normalOnlyChanged) message += "\"Normal Mode Deaths only\"";
							message += (useLocalChanged && normalOnlyChanged) ? " have" : " has";
							message += " changed.</c>\nFor this change to take effect, you need "
								"to <cl>quit and rejoin the level</c>.";
							if (useLocalChanged) message +=
								std::string("\nCurrently, you ") +
									(playLayer->m_fields->m_useLocal ? "<cj>are</c>" : "are <cr>not</c>")
									+ " using local deaths.";
							if (normalOnlyChanged) message +=
								std::string("\nCurrently, you ") +
									(playLayer->m_fields->m_normalOnly ? "<cj>only see normal mode</c>" : "see <cj>all</c>")
									+ " deaths.";

							FLAlertLayer::create(
								"Settings changed", message, "OK"
							)->show();
						}
					);
				}
			);

      button->setID("settings-button"_spr);
			button->setUserObject("alphalaneous.tooltips/tooltip",
				CCString::create("DeathMarkers Settings"));
			leftMenu->addChild(button);
			leftMenu->updateLayout(true);
		}

	}

};

$execute {

	auto mod = Mod::get();

	// Upgrade Settings
	int settingVersion;
	try {
		settingVersion = mod->getSavedValue<int>("setting-version");
	}
	catch (exception err) {
		log::debug("No value saved.");
		settingVersion = 0;
	}

	log::debug("Version {}", settingVersion);
	if (settingVersion < 0) settingVersion = 0;
	if (settingVersion == 0) {
		// Update "Use local deaths" to its new scheme
		mod->setSettingValue<std::string>("store-local-2",
			mod->getSettingValue<bool>("store-local") ? "Always" : "Never"
		);

		// Update "Always show" & "Show in practice" to the new scheme
		bool always = mod->getSettingValue<bool>("always-show");
		mod->setSettingValue<std::string>("condition-normal",
			always ? "Always" : "On Death"
		);
		mod->setSettingValue<std::string>("condition-practice",
			mod->getSettingValue<bool>("draw-in-practice") ?
				(always ? "Always" : "On Death") : "Never"
		);

		settingVersion = 1;
	}
	if (settingVersion > 1) settingVersion = 1;

	mod->setSavedValue("setting-version", settingVersion);

};
