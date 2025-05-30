#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <Geode/ui/MDPopup.hpp>
#include <vector>
#include <string>
#include <stdio.h>
#include "shared.hpp"

using namespace geode::prelude;
using namespace dm;

// FUNCTIONS

static bool shouldSubmit(struct playingLevel level, struct playerData player) {
	// Ignore Testmode and local Levels
	if (level.testmode) return false;
	if (level.levelId == 0) return false;

	if (player.userid == 0) return false;

	// Respect User Setting
	auto sharingEnabled = Mod::get()->getSettingValue<bool>("share-deaths");
	if (!sharingEnabled) return false;

	return true;
};

static bool shouldDraw(struct playingLevel level) {
	auto playLayer = PlayLayer::get();
	auto mod = Mod::get();

	if (!playLayer) return false;
	if (level.levelId == 0) return false; // Don't draw on local levels

	bool isPractice = level.practice || level.testmode;
	auto drawInPractice = mod->getSettingValue<bool>("draw-in-practice");
	if (isPractice && !drawInPractice) return false;

	float scale = mod->getSettingValue<float>("marker-scale");
	if (scale != 0) return true;

	int histHeight = mod->getSettingValue<int>("prog-bar-hist-height");
	if (histHeight != 0) return true;

	return true;
};

// MODIFY UI

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJGameLevel.hpp>
class $modify(DMPlayLayer, PlayLayer) {

	struct Fields {
		EventListener<web::WebTask> m_listener;

		CCNode* m_dmNode = CCNode::create();
		CCDrawNode* m_chartNode = nullptr;

		vector<DeathLocationMin> m_deaths;
		deque<DeathLocationOut> m_queuedSubmissions;

		bool m_chartAttached = false;
		bool m_fetched = false;
		struct playerData m_playerProps;
		struct playingLevel m_levelProps;
	};

	bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {

		if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

		// TODO: Info-button?

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

		// Don't even continue to list if we're not going to show them anyway
		if (!shouldDraw(this->m_fields->m_levelProps)) {
			this->m_fields->m_fetched = true;
			return true;
		}

		this->m_fields->m_deaths.clear();

		this->fetch(
			[this](bool success) {
				if (Mod::get()->getSettingValue<bool>("always-show")) {
					log::debug("Always show enabled, rendering...");
					this->renderMarkers(
						this->m_fields->m_deaths.begin(),
						this->m_fields->m_deaths.end()
					);
					this->renderHistogram();
				}

				this->checkQueue();
			}
		);

		this->schedule(schedule_selector(DMPlayLayer::updateMarkers), 0);

		return true;

	}

	// Sometimes this method creates the progress bar delayed
	void setupHasCompleted() {

		PlayLayer::setupHasCompleted();
		if (Mod::get()->getSettingValue<bool>("always-show")) this->renderHistogram();

	}

	void fetch(std::function<void(bool)> cb) {

		if (this->m_fields->m_fetched) return cb(true);

		log::info("Listing Deaths...");

		if (Mod::get()->getSettingValue<bool>("store-local")) {
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

		// Parse result JSON and add all as DeathLocationMin instances to playingLevel.deaths
		m_fields->m_listener.bind(
			[this, cb](web::WebTask::Event* const e) {
				auto res = e->getValue();
				if (res) {
					if (!res->ok()) {
						log::error("Listing Deaths failed: {}",
								   res->string().unwrapOr("Body could not be read."));
						cb(false);
					} else {
						log::debug("Received death list.");
						parseBinDeathList(res, &this->m_fields->m_deaths, !this->m_fields->m_levelProps.platformer);
						sort(
							this->m_fields->m_deaths.begin(),
							this->m_fields->m_deaths.end()
						);
						log::debug("Finished parsing.");
						this->m_fields->m_fetched = true;

						cb(true);
					}
				}
				else if (e->isCancelled()) {
					log::error("Death Listing Request was cancelled.");
					cb(false);
				};
			}
		);

		// Build the HTTP Request
		web::WebRequest req = web::WebRequest();

		req.param("levelid", this->m_fields->m_levelProps.levelId);
		req.param("platformer", this->m_fields->m_levelProps.platformer ? "true" : "false");
		req.param("practice", Mod::get()->getSettingValue<bool>("normal-only") ? "false" : "true");
		req.param("response", "bin");
		req.userAgent(HTTP_AGENT);
		req.timeout(HTTP_TIMEOUT);

		this->m_fields->m_listener.setFilter(req.get(dm::makeRequestURL("list")));

	}

	void resetLevel() {

		PlayLayer::resetLevel();

		if (Mod::get()->getSettingValue<bool>("always-show")) return;

		clearMarkers();

	}

	void levelComplete() {

		PlayLayer::levelComplete();
		if (this->m_fields->m_levelProps.platformer) return;

		auto deathLoc = DeathLocationOut(this->getPosition());
		deathLoc.percentage = 101;
		trySubmitDeath(deathLoc);

	}

	void togglePracticeMode(bool toggle) {

		PlayLayer::togglePracticeMode(toggle);
		this->m_fields->m_levelProps.practice = toggle;

		if (Mod::get()->getSettingValue<bool>("always-show") && (!toggle || Mod::get()->getSettingValue<bool>("draw-in-practice"))) {
			renderMarkers(
				this->m_fields->m_deaths.begin(),
				this->m_fields->m_deaths.end()
			);
			renderHistogram();
		} else clearMarkers();

	}

	void onQuit() {

		PlayLayer::onQuit();

		if (!Mod::get()->getSettingValue<bool>("store-local")) return;

		storeLocalDeaths(
			this->m_fields->m_levelProps.levelId,
			this->m_fields->m_deaths,
			!this->m_fields->m_levelProps.platformer
		);
		this->m_fields->m_deaths.clear();

	}

	void renderMarkers(const vector<DeathLocationMin>::iterator begin, const vector<DeathLocationMin>::iterator end) {

		double fadeTime = Mod::get()->getSettingValue<float>("fade-time") / 2;
		for (auto deathLoc = begin; deathLoc < end; deathLoc++) {
			auto node = deathLoc->createAnimatedNode(
				false,
				(static_cast<double>(rand()) / RAND_MAX) * fadeTime,
				fadeTime
			);
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
			child->setScale(inverseScale);
			child->setRotation(-sceneRotation);
		}

	}

	void renderHistogram() {

		int histHeight = Mod::get()->getSettingValue<int>("prog-bar-hist-height");

		// Only Draw Histogram if requested and applicable
		if (histHeight == 0 || this->m_fields->m_levelProps.platformer) return;

		int hist[101] = { 0 };

		for (auto& deathLoc : this->m_fields->m_deaths) {
			if (deathLoc.percentage >= 0 && deathLoc.percentage < 101)
				hist[deathLoc.percentage]++;
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

	void renderMarkersInFrame() {

		auto begin = this->m_fields->m_deaths.begin();
		auto end = this->m_fields->m_deaths.end();

		// For all this jargon, see the "Screen Limit" slide in docs/doc.dio
		auto winSize = CCDirector::sharedDirector()->getWinSize();
		auto halfWinWidth = winSize.width / 2;
		float winDiagonal =
			(sqrt(winSize.width * winSize.width + winSize.height * winSize.height)
			/ this->m_objectLayer->getScale() + 70) / 2;
		log::debug("{} {}", halfWinWidth, winDiagonal);

		begin = binarySearchNearestXPosOnScreen(begin, end, this->m_objectLayer,
			halfWinWidth - winDiagonal);
		end = binarySearchNearestXPosOnScreen(begin, end, this->m_objectLayer,
			halfWinWidth + winDiagonal);
		
		renderMarkers(begin, end);

	}

	void trySubmitDeath(DeathLocationOut const& deathLoc) {

		if (!shouldSubmit(
			this->m_fields->m_levelProps,
			this->m_fields->m_playerProps
		)) {
			if (!this->m_fields->m_fetched) {
				this->fetch([](bool _){});
			}
			return;
		}

		if (this->m_fields->m_queuedSubmissions.empty() &&
			this->m_fields->m_fetched) {
			this->submitDeath(deathLoc,
				[this, deathLoc](bool success) {
					if (!success) {
						this->m_fields->m_queuedSubmissions.push_front(deathLoc);
					}
				}
			);
		}
		else {
			this->m_fields->m_queuedSubmissions.push_back(deathLoc);
			this->checkQueue();
		}

	}

	void submitDeath(DeathLocationOut const& deathLoc,
		std::function<void(bool)> cb) {

		auto mod = Mod::get();
		auto playLayer = static_cast<DMPlayLayer*>(
			GameManager::get()->getPlayLayer()
		);

		m_fields->m_listener.bind(
			[this, deathLoc, cb](web::WebTask::Event* e) {
				auto res = e->getValue();
				if (res) {
					if (!res->ok()) {
						int code = res->code();
						auto body = res->string().unwrapOr("Unknown");
						log::error("Posting Death failed: {} {}", code, body);
						if (code == 429) {
							bool queued = !this->m_fields->m_queuedSubmissions.empty();
							if (queued) this->m_fields->m_queuedSubmissions.clear();
							this->spamWarning(body.starts_with("IP"), queued);
						}
						cb(false);
					}
					else {
						log::debug("Posted Death.");
						cb(true);
					}
				}
				else if (e->isCancelled()) {
					log::error("Posting Death was cancelled");
					cb(false);
				}
			}
		);

		// Build the HTTP Request
		auto myjson = matjson::Value();
		myjson.set("levelid", matjson::Value(
			playLayer->m_level->m_levelID.value()
		));
		myjson.set("levelversion", matjson::Value(
			playLayer->m_level->m_levelVersion
		));
		myjson.set("practice", matjson::Value(
			playLayer->m_isPracticeMode
		));
		myjson.set("playername", matjson::Value(
			this->m_fields->m_playerProps.username
		));
		myjson.set("userid", matjson::Value(this->m_fields->m_playerProps.userid));
		myjson.set("format", matjson::Value(FORMAT_VERSION));
		deathLoc.addToJSON(&myjson);

		web::WebRequest req = web::WebRequest();
		req.param("levelid", playLayer->m_level->m_levelID.value());
		req.bodyJSON(myjson);

		req.userAgent(HTTP_AGENT);

		req.header("Content-Type", "application/json");
		req.header("Accept", "application/json");

		req.timeout(HTTP_TIMEOUT);

		m_fields->m_listener.setFilter(req.get(dm::makeRequestURL("submit")));

	}

	void spamWarning(bool banned, bool queued) {
		this->pauseGame(false);
		std::string content = banned ?
"<cy>You have been <cr>blocked from submitting deaths</c> for the next hour. For \
everyone's convenience, submissions from your game have been automatically \
disabled in the mod settings." :
			"<cy>Your death was just denied for breaking \
the rate limit.</c> If you did not do this intentionally, please <cg>wait a few \
seconds</c> after this warning, to ensure you are not rate limited again. If you \
do, you will be <cr>banned from submitting deaths for an hour</c> to prevent spam \
requests to the server. Thanks for understanding.";
		if (!banned && queued) content += "\n\nThis may have been a result of the mod submitting \
queued deaths while the server was offline. Normally, this should not get you rate-\
limited, but for your safety, the death backlog has been forcefully emptied.";
		content += "\n\nIf you are using a VPN, you may be rate limited because you \
share an IP with others. I strongly advise you to turn it off.";
		MDPopup::create(
			"Death Markers Spam Notice",
			content,
			"Dismiss"
		)->show();
		if (banned) Mod::get()->setSettingValue("share-deaths", false);
	}

	void checkQueue() {

		if (this->m_fields->m_queuedSubmissions.empty()) return;

		if (this->m_fields->m_fetched) {
			log::debug(
				"Clearing Queue. {} deaths pending.",
				this->m_fields->m_queuedSubmissions.size()
			);

			DeathLocationOut next = this->m_fields->m_queuedSubmissions.front();
			this->m_fields->m_queuedSubmissions.pop_front();

			this->submitDeath(next,
				[this, next](bool success) {
					if (success) {
						this->checkQueue();
					} else {
						this->m_fields->m_queuedSubmissions.push_front(next);
					}
				}
			);
		} else {
			log::debug("Attempting to fetch...");

			this->fetch(
				[this](bool success) {
					if (success) {
						this->checkQueue();
					}
				}
			);
		}
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
		auto deathLoc = DeathLocationOut(this->getPosition());
		deathLoc.percentage = percent;
		// deathLoc.coin1 = ...; // This stuff is complicated... prolly gonna pr Weebifying/coins-in-pause-menu-geode to make it api public and depend on it here or sm
		// deathLoc.coin2 = ...;
		// deathLoc.coin3 = ...;
		// deathLoc.itemdata = ...; // where the hell are the counters

		playLayer->trySubmitDeath(deathLoc);

		bool newBest = playLayer->m_fields->m_levelProps.platformer || (
			playLayer->getCurrentPercentInt() >= playLayer->m_level->m_normalPercent.value() ||
			playLayer->m_level->m_normalPercent.value() == 100
		);
		auto render = shouldDraw(playLayer->m_fields->m_levelProps) &&
			(!Mod::get()->getSettingValue<bool>("newbest-only") || newBest);

		// Render Death Markers
		if (render) {
			double fadeTime = Mod::get()->getSettingValue<float>("fade-time") / 2;

			if (Mod::get()->getSettingValue<bool>("always-show")) {
				playLayer->m_fields->m_dmNode->addChild(
					deathLoc.createAnimatedNode(false, 0, fadeTime)
				);
			}
			else {
				playLayer->renderMarkersInFrame();
				playLayer->m_fields->m_dmNode->addChild(
					deathLoc.createAnimatedNode(true, 0, fadeTime)
				);
			}
		}

		// Add own death to current level's list
		// after rendering because the current death's CCNode is being rendered separately
		if (!Mod::get()->getSettingValue<bool>("normal-only")
			|| !playLayer->m_fields->m_levelProps.platformer)
			playLayer->m_fields->m_deaths.push_back(deathLoc);

		if (render) playLayer->renderHistogram();

	}

};
