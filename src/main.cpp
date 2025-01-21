#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <vector>
#include <string>
#include <stdio.h>
#include <geode/Utils.hpp>

// TODO: Parse result from list
// TODO: "always-show" setting

using namespace geode::prelude;

std::string const HTTP_AGENT =
"Geode-DeathMarkers-" + Mod::get()->getVersion().toVString(true);
auto const FORMAT_VERSION = 1;

// If you fork this mod and change the code, you should change this to the localhost
//std::string const API_BASE = "https://deathmarkers.masp005.dev/";
std::string const API_BASE = "http://localhost:8048/";
auto const HTTP_TIMEOUT = std::chrono::seconds(15);

// CLASSES

class DeathLocationMin {
public:
	CCPoint pos;
	int percentage;

	DeathLocationMin(float x, float y, int percentage) {
		this->pos = CCPoint(x, y);
		this->percentage = percentage;
	}

	DeathLocationMin(CCPoint pos, int percentage) {
		this->pos = CCPoint(pos);
		this->percentage = percentage;
	}

	CCNode* createNode(bool isCurrent) const {
		return this->createNode(isCurrent, false);
	}

	CCNode* createAnimatedNode(bool isCurrent, double delay) const {
		auto node = this->createNode(isCurrent, true);
		node->runAction(CCSequence::createWithTwoActions(
			CCDelayTime::create(delay),
			CCSpawn::createWithTwoActions(
				CCEaseBounceOut::create(
					CCMoveTo::create(0.25f, this->pos)
				),
				CCFadeIn::create(0.25f)
			)
		));
		return node;
	}

	CCNode* createNode(bool isCurrent, bool preAnim) const {
		auto sprite = CCSprite::create("death-marker.png"_spr);
		std::string const id = "marker"_spr;
		float markerScale = Mod::get()->getSettingValue<float>("marker-scale");
		if (isCurrent) {
			sprite->setScale(markerScale * 1.5);
			sprite->setZOrder(2 << 29);
		}
		else {
			sprite->setScale(markerScale);
			sprite->setZOrder(2 << 29 - 1);
		}
		if (preAnim) {
			auto point = CCPoint(this->pos.x, this->pos.y + markerScale * 4);
			sprite->setPosition(point);
			sprite->setOpacity(0);
		}
		else {
			sprite->setPosition(this->pos);
		}
		sprite->setZOrder(2 << 28 - !isCurrent);
		sprite->setAnchorPoint({ 0.5f, 0.0f });
		return sprite;
	}
};

// FYI i too would like to inherit DeathLocationMin here but c++ is messing around with it
class DeathLocationOut {
public:
	CCPoint pos;
	int percentage = 0;
	bool coin1 = false;
	bool coin2 = false;
	bool coin3 = false;
	int itemdata = 0;

	DeathLocationOut(CCPoint pos, int percentage) {
		this->pos = CCPoint(pos);
		this->percentage = percentage;
	}

	DeathLocationMin toMin() const {
		return DeathLocationMin(this->pos, percentage);
	}

	void addToJSON(matjson::Value* json) const {
		json->set("x", matjson::Value(this->pos.x));
		json->set("y", matjson::Value(this->pos.y));
		json->set("percentage", matjson::Value(this->percentage));
		json->set("coins", matjson::Value(this->coin1 | this->coin2 << 1 | this->coin3 << 2));
		json->set("itemdata", matjson::Value(this->itemdata));
	}
};

class DeathLocation {
public:
	std::string userIdent;
	CCPoint pos;
	int percentage = 0;
	bool coin1 = false;
	bool coin2 = false;
	bool coin3 = false;
	int itemdata = 0;

	DeathLocation(float x, float y) {
		this->pos = CCPoint(x, y);
	}

	DeathLocation(CCPoint pos) {
		this->pos = CCPoint(pos);
	}

	DeathLocationMin toMin() const {
		return DeathLocationMin(this->pos, this->percentage);
	}
};

class DeathsAnalyser {
protected:
	DeathsAnalyser(std::vector<DeathLocation>& deaths) {
		this->deaths = deaths;
	}

	std::vector<DeathLocation> deaths;

public:
	void setDeaths(std::vector<DeathLocation>& deaths) {
		this->deaths = deaths;
	}
};

// DATA HOLDERS

struct {
	std::string username = "";
	long int userid = 0;
} playerData;

struct {
	long int levelid = 0;
	int levelversion = 0;
	cocos2d::CCCamera* camera = nullptr;
	bool practice = false;
	bool platformer = false;
	bool testmode = false;
} playingLevel;

// FUNCTIONS

static bool shouldSubmit() {
	// Ignore Testmode and local Levels
	if (playingLevel.testmode) return false;
	if (playingLevel.levelid == 0) return false;

	if (playerData.userid == 0) return false;

	// Respect User Setting
	auto sharingEnabled = Mod::get()->getSettingValue<bool>("share-deaths");
	if (!sharingEnabled) return false;

	return true;
};

static bool shouldDraw() {
	auto playLayer = PlayLayer::get();
	auto mod = Mod::get();

	if (!playLayer) return false;
	if (playingLevel.levelid == 0) return false; // Don't draw on local levels

	bool isPractice = playingLevel.practice || playingLevel.testmode;
	auto drawInPractice = mod->getSettingValue<bool>("draw-in-practice");
	if (isPractice && !drawInPractice) return false;

	float scale = mod->getSettingValue<float>("marker-scale");
	if (scale != 0) return true;

	int histHeight = mod->getSettingValue<int>("prog-bar-hist-height");
	if (histHeight != 0) return true;

	return true;
};

// TODO: stuff into EditLayer
static void analyseDeaths(DeathsAnalyser* analyser) {
	std::vector<DeathLocation> list;
	std::string url = API_BASE + "analyse";
	// GET /analyse
	// write into *analyser.deaths;
};

// MODIFY UI

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJGameLevel.hpp>
class $modify(DMPlayLayer, PlayLayer) {

	struct Fields {
		EventListener<web::WebTask> m_listener;
		CCNode* m_dmNode = CCNode::create();
		CCDrawNode* m_chartNode = nullptr;
		bool m_chartAttached = false;
		std::vector<DeathLocationMin> m_deaths;
	};

	bool init(GJGameLevel * level, bool useReplay, bool dontCreateObjects) {

		if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

		// Prepare UI
		this->m_fields->m_dmNode->setID("markers"_spr);
		this->m_fields->m_dmNode->setZOrder(2 << 28); // everyone using 99999 smh
		this->m_objectLayer->addChild(this->m_fields->m_dmNode);

		// refetch on level start in case it changed
		playerData.userid = GameManager::get()->m_playerUserID.value();
		playerData.username = GameManager::get()->m_playerName;

		auto mod = Mod::get();

		// save level data
		playingLevel.camera = level->getCamera();
		playingLevel.levelid = level->m_levelID;
		playingLevel.levelversion = level->m_levelVersion;
		playingLevel.platformer = level->isPlatformer();
		playingLevel.practice = this->m_isPracticeMode;
		playingLevel.testmode = this->m_isTestMode;

		// Don't even continue to list if we're not going to show them anyway
		if (!shouldDraw()) return true;

		log::info("Listing Deaths...");
		this->m_fields->m_deaths.clear();

		// Parse result JSON and add all as DeathLocationMin instances to playingLevel.deaths
		m_fields->m_listener.bind([mod, this](web::WebTask::Event* const e) {
			auto res = e->getValue();
			if (res) {
				if (!res->ok())
					log::error("Listing Deaths failed: {}", res->string().unwrapOr("Body could not be read."));
				else {
					log::info("Received something: {}",
						res->string().unwrapOr("Body could not be read."));

					auto const body = res->json();
					if (body.isErr())
						return log::error("Error reading list response: Body could not be read.");

					auto const okObj = body.ok();
					matjson::Value parsed = okObj.value();
					if (!parsed.isArray())
						return log::error("Unexpected Non-Array response listing deaths: {}", parsed.dump(matjson::NO_INDENTATION));

					auto const& list = parsed.asArray().unwrap(); // [[#,#,#],[#,#,#],...]
					for (int i = 0; i < list.size(); i++) {
						auto const& item = list.at(i);
						if (!item.isArray())
							return log::error("Unexpected Non-Array item listing deaths: {}", item.dump(matjson::NO_INDENTATION));

						auto const& coords = item.asArray().unwrap(); // [#,#,#]
						if (coords.size() != 3)
							return log::error("Unexpected Non-3-Tuple item listing deaths: {}, size {}", item.dump(matjson::NO_INDENTATION), coords.size());

						auto const& x = coords.at(0);
						auto const& y = coords.at(1);
						auto const& percentage = coords.at(2);
						if (!x.isNumber() || !y.isNumber() || !percentage.isNumber())
							return log::error("Unexpected Non-Number coordinate listing deaths: {}", item.dump(matjson::NO_INDENTATION));

						auto deathLoc = DeathLocationMin(x.asDouble().unwrap(), y.asDouble().unwrap(), percentage.asInt().unwrap());
						this->m_fields->m_deaths.push_back(deathLoc);
					}
				}
			}
			else if (e->isCancelled()) {
				log::error("Death Listing Request was cancelled.");
			};
			});

		// Build the HTTP Request
		std::string const url = API_BASE + "list";
		web::WebRequest req = web::WebRequest();

		req.param("levelid", (playingLevel.levelid));
		req.userAgent(HTTP_AGENT);
		req.timeout(HTTP_TIMEOUT);

		this->m_fields->m_listener.setFilter(req.get(url));

		return true;

	};

	void resetLevel() {

		PlayLayer::resetLevel();

		m_fields->m_dmNode->removeAllChildrenWithCleanup(true);
		m_fields->m_dmNode->cleanup();

		if (!this->m_fields->m_chartAttached) return;
		this->m_fields->m_chartNode->clear();
		this->m_fields->m_chartNode->cleanup();

	}

	void levelComplete() {

		PlayLayer::levelComplete();
		if (playingLevel.platformer) return;

		auto deathLoc = DeathLocationOut(this->getPosition(), 101);
		submitDeath(deathLoc);

	}

	void togglePracticeMode(bool toggle) {

		PlayLayer::togglePracticeMode(toggle);
		playingLevel.practice = toggle;

	}

	void onQuit() {

		this->m_fields->m_deaths.clear();
		PlayLayer::onQuit();

	}

	void renderDeaths() {

		int histHeight = Mod::get()->getSettingValue<int>("prog-bar-hist-height");
		int hist[101] = { 0 };

		for (auto& deathLoc : this->m_fields->m_deaths) {
			auto node = deathLoc.createAnimatedNode(false, (static_cast<float>(rand()) / RAND_MAX) * .25f);
			this->m_fields->m_dmNode->addChild(node);
			hist[deathLoc.percentage]++;
		}

		// Only Draw Histogram if requested
		if (histHeight == 0) return;

		if (!this->m_fields->m_chartAttached) {
			auto progBarNode = this->getChildByID("progress-bar");
			if (progBarNode == nullptr) return;

			this->m_fields->m_chartNode = CCDrawNode::create();
			this->m_fields->m_chartNode->setID("chart"_spr);
			this->m_fields->m_chartNode->setZOrder(-2);
			this->m_fields->m_chartNode->setPosition(2, 4);
			this->m_fields->m_chartNode->setContentWidth(progBarNode->getContentWidth() - 4);
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

			float distrib = static_cast<float>(hist[i]) / maximum;
			auto rect = CCRect(width * i, 0, width, -(distrib * histHeight));
			auto color = _ccColor4F(distrib, 1 - distrib, 0, 1);

			this->m_fields->m_chartNode->drawRect(rect, color, 0.0f, color);
		}

	}

	void submitDeath(DeathLocationOut const& deathLoc) {

		auto mod = Mod::get();
		auto playLayer = static_cast<DMPlayLayer*>(GameManager::get()->getPlayLayer());

		m_fields->m_listener.bind([mod](web::WebTask::Event* e) {
			auto res = e->getValue();
			if (res) {
				if (!res->ok())
					log::error(
						"Posting Death failed: {}",
						res->string().unwrapOr("Body could not be read.")
					);
				else log::info("Posted Death.");
			}
			else if (e->isCancelled())
				log::error("Posting Death was cancelled");
			});

		// Build the HTTP Request
		std::string const url = API_BASE + "submit";
		auto myjson = matjson::Value();
		myjson.set("levelid", matjson::Value(static_cast<int>(playLayer->m_level->m_levelID)));
		myjson.set("levelversion", matjson::Value(playLayer->m_level->m_levelVersion));
		myjson.set("practice", matjson::Value(playLayer->m_isPracticeMode));
		myjson.set("playername", matjson::Value(playerData.username));
		myjson.set("userid", matjson::Value(playerData.userid));
		myjson.set("format", matjson::Value(FORMAT_VERSION));
		deathLoc.addToJSON(&myjson);
		log::info("JSON body: {}", myjson.dump(matjson::NO_INDENTATION));

		web::WebRequest req = web::WebRequest();
		req.bodyJSON(myjson);

		req.userAgent(HTTP_AGENT);

		req.header("Content-Type", "application/json");
		req.header("Accept", "application/json");

		req.timeout(HTTP_TIMEOUT);

		auto task = req.get(url);
		m_fields->m_listener.setFilter(task);

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

		auto playLayer = static_cast<DMPlayLayer*>(GameManager::get()->getPlayLayer());
		if (!playLayer) return;

		// Populate percentage as current time or progress percentage
		int percent = playingLevel.platformer ?
			static_cast<int>(playLayer->m_attemptTime) :
			playLayer->getCurrentPercentInt();
		auto deathLoc = DeathLocationOut(this->getPosition(), percent);
		// deathLoc.coin1 = ...; // This stuff is complicated... prolly gonna pr Weebifying/coins-in-pause-menu-geode to make it api public and depend on it here or sm
		// deathLoc.coin2 = ...;
		// deathLoc.coin3 = ...;
		// deathLoc.itemdata = ...; // where the hell are the counters

		if (shouldSubmit()) playLayer->submitDeath(deathLoc);

		// Render Death Markers
		if (shouldDraw()) {
			playLayer->renderDeaths();
			playLayer->m_fields->m_dmNode->addChild(deathLoc.toMin().createAnimatedNode(true, 0));
		}

		// Add own death to current level's list
		// after rendering because the current death's CCNode already exists
		playLayer->m_fields->m_deaths.push_back(deathLoc.toMin());

	}

};