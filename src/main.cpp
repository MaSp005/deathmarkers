#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <vector>
#include <string>
#include <stdio.h>
#include "shared.hpp"

// TODO: resources as spritesheets
// TODO: Check for incompatibilities with other mods

using namespace geode::prelude;

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

		// TODO: Info-button

		// Prepare UI
		this->m_fields->m_dmNode->setID("markers"_spr);
		this->m_fields->m_dmNode->setZOrder(2 << 28); // everyone using 99999 smh
		this->m_objectLayer->addChild(this->m_fields->m_dmNode);

		// refetch on level start in case it changed
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
					log::info("Always show enabled, rendering...");
					this->renderMarkers();
				}

				this->checkQueue();
			}
		);

		return true;

	}

	void fetch(std::function<void(bool)> cb) {

		if (this->m_fields->m_fetched) return cb(true);
		
		log::info("Listing Deaths...");

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
						log::info("Received death list.");
						parseDeathList(res, &this->m_fields->m_deaths);
						log::info("Finished parsing.");
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
		std::string const url = API_BASE + "list";
		web::WebRequest req = web::WebRequest();

		req.param("levelid", this->m_fields->m_levelProps.levelId);
		req.param("platformer", this->m_fields->m_levelProps.platformer);
		req.param("response", "csv");
		req.userAgent(HTTP_AGENT);
		req.timeout(HTTP_TIMEOUT);

		this->m_fields->m_listener.setFilter(req.get(url));

	}

	void resetLevel() {

		PlayLayer::resetLevel();

		if (Mod::get()->getSettingValue<bool>("always-show")) return;

		m_fields->m_dmNode->removeAllChildrenWithCleanup(true);
		m_fields->m_dmNode->cleanup();

		if (!this->m_fields->m_chartAttached) return;
		this->m_fields->m_chartNode->clear();
		this->m_fields->m_chartNode->cleanup();

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

	}

	void onQuit() {

		this->m_fields->m_deaths.clear();
		PlayLayer::onQuit();

	}

	void renderMarkers() {

		for (auto& deathLoc : this->m_fields->m_deaths) {
			auto node = deathLoc.createAnimatedNode(
				false,
				(static_cast<double>(rand()) / RAND_MAX) * .25f
			);
			this->m_fields->m_dmNode->addChild(node);
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
			auto progBarNode = this->getChildByID("progress-bar");
			if (progBarNode == nullptr) return;

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
			ccColor4F color{};
			color.r = distr;
			color.g = 1 - distr;
			color.b = 0;
			color.a = 1;

			this->m_fields->m_chartNode->drawRect(rect, color, 0.0f, color);
		}

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
						log::error(
							"Posting Death failed: {}",
							res->string().unwrapOr("Body could not be read.")
						);
						cb(false);
					}
					else {
						log::info("Posted Death.");
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
		std::string const url = API_BASE + "submit";
		auto myjson = matjson::Value();
		myjson.set("levelid", matjson::Value(
			static_cast<int>(playLayer->m_level->m_levelID)
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
		req.bodyJSON(myjson);

		req.userAgent(HTTP_AGENT);

		req.header("Content-Type", "application/json");
		req.header("Accept", "application/json");

		req.timeout(HTTP_TIMEOUT);

		auto task = req.get(url);
		m_fields->m_listener.setFilter(task);

	}

	void checkQueue() {

		if (this->m_fields->m_queuedSubmissions.empty()) return;
		
		if (this->m_fields->m_fetched) {
			log::info(
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
			log::info("Attempting to fetch...");

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

	static void onModify(auto & self) {

		// Hook before QOLMod (-6969) hook that completely overrides playerDestroyed
		if (!self.setHookPriority("PlayerObject::playerDestroyed", -6970)) {
			log::error("Failed to set hook priority of PlayerObject::playerDestroyed to -6970 (somehow)");
		}

	}

	void playerDestroyed(bool secondPlr) {

		// Forward to original, we dont want noclip in here
		PlayerObject::playerDestroyed(secondPlr);
		if (secondPlr) return;

		auto playLayer = static_cast<DMPlayLayer*>(
			GameManager::get()->getPlayLayer()
		);
		if (!playLayer) return;

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

		auto render = shouldDraw(playLayer->m_fields->m_levelProps);

		// Render Death Markers
		if (render) {
			if (Mod::get()->getSettingValue<bool>("always-show")) {
				playLayer->m_fields->m_dmNode->addChild(
					deathLoc.createAnimatedNode(false, 0)
				);
			}
			else {
				playLayer->renderMarkers();
				playLayer->m_fields->m_dmNode->addChild(
					deathLoc.createAnimatedNode(true, 0)
				);
			}
		}

		// Add own death to current level's list
		// after rendering because the current death's CCNode is being rendered separately
		playLayer->m_fields->m_deaths.push_back(deathLoc);

		if (render) playLayer->renderHistogram();

	}

};