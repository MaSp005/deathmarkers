// Include utility headers
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <vector>
#include <string>

// TODO: Parse result from list
// TODO: Bar chart thingie on progress bar (and setting)
// TODO: death markers themselves?
// TODO: 100% -> 99%
// TODO: send completion "death"

using namespace geode::prelude;

std::string const HTTP_AGENT =
"Geode-DeathMarkers-" + Mod::get()->getVersion().toVString(true);
//std::string const API_BASE = "https://deathmarkers.masp005.dev/";
std::string const API_BASE = "http://localhost:8048/";
auto const HTTP_TIMEOUT = std::chrono::seconds(15);

// CLASSES

class DeathLocationMin {
public:
	float x;
	float y;

	DeathLocationMin(float x, float y) {
		this->x = x;
		this->y = y;
	}

	CCNode* createNode() {
		auto sprite = CCSprite::create("death-marker.png"_spr);
		sprite->setID("marker"_spr);
		sprite->setPositionX(this->x);
		sprite->setPositionY(this->y);
		return sprite;
	}
};

class DeathLocationOut {
public:
	DeathLocationOut(float x, float y) {
		this->x = x;
		this->y = y;
	}

	float x;
	float y;
	int percentage = 0;
	bool coin1 = false;
	bool coin2 = false;
	bool coin3 = false;
	int itemdata = 0;

	DeathLocationMin toMin() const {
		return DeathLocationMin(this->x, this->y);
	}

	void addToJSON(matjson::Value* json) const {
		json->set("x", matjson::Value(this->x));
		json->set("y", matjson::Value(this->y));
		json->set("percentage", matjson::Value(this->percentage));
		json->set("coins", matjson::Value(this->coin1 | this->coin2 << 1 | this->coin3 << 2));
		json->set("itemdata", matjson::Value(this->itemdata));
	}
};

class DeathLocation {
public:
	std::string userIdent;
	float x;
	float y;
	int percentage;
	bool coin1;
	bool coin2;
	bool coin3;
	int itemdata;

	DeathLocationMin toMin() const {
		return DeathLocationMin(this->x, this->y);
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
	std::vector<DeathLocationMin> deaths;
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

	// Respect User Setting
	auto sharingEnabled = Mod::get()->getSettingValue<bool>("share-deaths");
	if (!sharingEnabled) return false;

	return true;
};

static bool shouldDraw() {
	auto playLayer = PlayLayer::get();
	if (!playLayer) return false;
	if (playingLevel.levelid == 0) return false;

	auto mod = Mod::get();
	auto scale = mod->getSettingValue<double>("marker-scale");
	if (scale == 0) return false;

	// TODO: Settings for whether to draw in practice etc

	return true;
};

static void analyseDeaths(DeathsAnalyser* analyser) {
	std::vector<DeathLocation> list;
	std::string url = API_BASE + "analyse";
	// TODO: fetch stuff
	// GET /analyse
	// write into *analyser.deaths;
};

// MODIFY UI

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJGameLevel.hpp>
class $modify(DeathMarkersPlayLayer, PlayLayer) {

	struct Fields {
		EventListener<web::WebTask> m_listener;
		CCNode* m_dmNode;
	};

	bool init(GJGameLevel * level, bool useReplay, bool dontCreateObjects) {

		if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

		// refetch on level start in case it changed
		playerData.userid = GameManager::get()->m_playerUserID.value();
		playerData.username = GameManager::get()->m_playerName.data();

		auto mod = Mod::get();

		// for figuring out what to draw
		playingLevel.camera = level->getCamera();
		playingLevel.levelid = level->m_levelID;
		playingLevel.levelversion = level->m_levelVersion;
		playingLevel.platformer = level->isPlatformer();
		playingLevel.practice = this->m_isPracticeMode;
		playingLevel.testmode = this->m_isTestMode;

		if (mod->getSettingValue<bool>("console-debug")) log::info("Listing Deaths...");

		std::vector<DeathLocationMin> list;

		m_fields->m_listener.bind([mod, this](web::WebTask::Event* const e) {
			auto res = e->getValue();
			if (res) {
				if (!res->ok()) {
					log::error("Listing Deaths failed: {}", res->string().unwrapOr("Body could not be read."));
				} else {
					if (Mod::get()->getSettingValue<bool>("console-debug"))
						log::info("Recieved something: {}",
							res->string().unwrapOr("Body could not be read."));

					auto decoded = res->json();
					if (decoded.isErr())
						return log::error("Error reading list response: Body could not be read.");

					auto const result = decoded.ok().value();
					DeathMarkersPlayLayer::parseDeathList(result);
						
					// TODO: Add each as CCNode to PlayLayer
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

		// Prepare UI
		auto dmNode = CCNode::create();
		dmNode->setID("markers"_spr);
		this->m_fields->m_dmNode = dmNode;
		this->addChild(this->m_fields->m_dmNode);

		return true;

	};

	void parseDeathList(matjson::Value const& list) {

		if (!list.isArray())
			return log::error("Unexpected Non-Array response listing deaths: {}", list.dump(matjson::NO_INDENTATION));

		auto items = list.asArray().unwrap(); // [[#,#],[#,#],...]

		for (int i = 0; i < items.size(); i++) {
			auto item = items.at(i);
			if (!list.isArray())
				return log::error("Unexpected Non-Array item listing deaths: {}", item.dump(matjson::NO_INDENTATION));

			auto coords = item.asArray().unwrap(); // [#, #]
			if (coords.size() != 2)
				return log::error("Unexpected Non-2-Tuple item listing deaths: {}, size {}", item.dump(matjson::NO_INDENTATION), coords.size());

			auto x = coords.at(0);
			auto y = coords.at(1);
			if (!x.isNumber() || !y.isNumber())
				return log::error("Unexpected Non-Number coordinate listing deaths: {}", item.dump(matjson::NO_INDENTATION));

			auto deathLoc = DeathLocationMin(x.asDouble().unwrap(), y.asDouble().unwrap());
			this->addDeathMarker(deathLoc);
			playingLevel.deaths.push_back(deathLoc);
		}

	}

	void addDeathMarker(DeathLocationMin deathLoc) {
		this->m_fields->m_dmNode->addChild(deathLoc.createNode());
	}

	void togglePracticeMode(bool toggle) {

		PlayLayer::togglePracticeMode(toggle);
		playingLevel.practice = toggle;

	}

}
;

#include <Geode/modify/PlayerObject.hpp>
class $modify(PlayerObject) {

	struct Fields {
		EventListener<web::WebTask> m_listener;
	};

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

		auto playLayer = static_cast<DeathMarkersPlayLayer*>(GameManager::get()->getPlayLayer());
		if (!playLayer) return;

		auto mod = Mod::get();
		auto deathLoc = DeathLocationOut(this->getPositionX(), this->getPositionY());
		deathLoc.percentage = playLayer->getCurrentPercentInt();
		// deathloc.coin1 = ...;
		// deathloc.coin2 = ...;
		// deathloc.coin3 = ...;
		// deathloc.itemdata = ...;

		// Add own death to current level's list
		playingLevel.deaths.push_back(deathLoc.toMin());
		// TODO: Add as CCNode to PlayLayer

		if (shouldSubmit()) {

			m_fields->m_listener.bind([mod](web::WebTask::Event* e) {
				auto res = e->getValue();
				if (res) {
					if (!res->ok())
						log::error(
							"Posting Death failed: {}",
							res->string().unwrapOr("Body could not be read.")
						);
					else if (mod->getSettingValue<bool>("console-debug")) log::info("Posted Death.");
				}
				else if (e->isCancelled())
					log::error("Posting Death was cancelled");
				});

			// Build the HTTP Request
			std::string const url = API_BASE + "submit";
			auto myjson = matjson::Value();
			myjson.set("levelid", matjson::Value(playingLevel.levelid));
			myjson.set("levelversion", matjson::Value(playingLevel.levelversion));
			myjson.set("practice", matjson::Value(playingLevel.practice));
			myjson.set("playername", matjson::Value(playerData.username));
			myjson.set("userid", matjson::Value(playerData.userid));
			deathLoc.addToJSON(&myjson);
			if (mod->getSettingValue<bool>("console-debug")) log::info("JSON body: {}", myjson.dump(matjson::NO_INDENTATION));

			web::WebRequest req = web::WebRequest();
			req.bodyJSON(myjson);

			req.param("levelid", (playingLevel.levelid));
			req.param("levelversion", (playingLevel.levelversion));
			req.param("practice", (playingLevel.practice));
			req.param("playername", (playerData.username));
			req.param("userid", (playerData.userid));

			req.userAgent(HTTP_AGENT);

			req.header("Content-Type", "application/json");
			req.header("Accept", "application/json");

			req.timeout(HTTP_TIMEOUT);

			auto task = req.get(url);
			m_fields->m_listener.setFilter(task);

		}

		// Render Death Markers
		if (!shouldDraw()) return;

		//	playLayer->m_fields->m_respawnTimeSum = 0; // super hacky
		//	playLayer->m_fields->m_deathSprites->runAction(
		//		CCRepeatForever::create(CCSequence::createWithTwoActions(
		//			CCDelayTime::create(0),
		//			CCCallFunc::create(playLayer, callfunc_selector(ModifiedPlayLayer::updateCalcRespawnTime)))
		//		)
		//	);

		//	auto& deathPoints = playLayer->m_fields->m_deathPoints;
		//	auto deathSprites = playLayer->m_fields->m_deathSprites;
		//	deathPoints.push_back(this->getPosition());

		//	float& totalTime = playLayer->m_fields->m_deathMarkerAnimTime;
		//	totalTime = 0;

		//	bool animateMarkers = mod->getSettingValue<bool>("animate-markers");
		//	double markerScale = mod->getSettingValue<double>("marker-scale");
		//	double markerScaleThisDeath = markerScale * mod->getSettingValue<double>("marker-scale-thisdeath");

		//	DeathPoints visiblePoints; visiblePoints.reserve(deathPoints.size());
		//	int thisDeathIdx = -1;
		//	for (auto iter = deathPoints.rbegin(); iter != deathPoints.rend(); iter++) {
		//		auto& point = *iter;
		//		if (shouldRender(point, deathSprites)) {
		//			if (std::distance(iter, deathPoints.rbegin()) == 0) {
		//				thisDeathIdx = 0;
		//			}

		//			visiblePoints.push_back(point);
		//		}
		//	}

		//	int idx = 0;
		//	float interval = mod->getSettingValue<double>("marker-anim-time") / visiblePoints.size();
		//	for (auto& point : visiblePoints) {
		//		auto sprite = CCSprite::create("death-marker.png"_spr);
		//		sprite->setScale(markerScale);
		//		sprite->setAnchorPoint({ 0.5f, 0.0f });

		//		if (idx == thisDeathIdx) {
		//			sprite->setZOrder(99999);
		//			sprite->setScale(markerScaleThisDeath);
		//		}

		//		if (animateMarkers)
		//		{
		//			sprite->setPosition(point + CCPoint(0.0f, 20.0f));
		//			sprite->setOpacity(0);
		//			sprite->runAction(CCSequence::createWithTwoActions(
		//				CCDelayTime::create(idx * interval),
		//				CCSpawn::createWithTwoActions(
		//					CCMoveTo::create(0.25f, point),
		//					CCFadeIn::create(0.25f)
		//				)
		//			));
		//			totalTime += interval;
		//		}
		//		else {
		//			sprite->setPosition(point);
		//		}

		//		deathSprites->addChild(sprite);
		//		idx++;
		//	}

		//	totalTime += mod->getSettingValue<double>("marker-time");
		//}
	}

};

// PauseLayer -> left-button-menu
// PlayLayer -> progress-bar