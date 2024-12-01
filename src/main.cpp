/**
 * Include the Geode headers.
 */
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <vector>
#include <string>

using namespace geode::prelude;

std::string const HTTP_AGENT =
"Geode-DeathMarkers-" + Mod::get()->getVersion().toVString(true);
std::string const API_BASE = "https://deathmarkers.masp005.dev/";

// CLASSES

class DeathLocationMin {
public:
	double x;
	double y;
};

class DeathLocationOut : public DeathLocationMin {
public:
	double x;
	double y;
	int percentage;
	bool coin1;
	bool coin2;
	bool coin3;
	int itemdata;
};

class DeathLocation : public DeathLocationMin {
public:
	std::string userIdent;
	double x;
	double y;
	int percentage;
	bool coin1;
	bool coin2;
	bool coin3;
	int itemdata;
};

class DeathsAnalyser {
	std::vector<DeathLocation> deaths;
};

// DATA HOLDERS

struct {
	std::string username;
	long int accountid;
} playerData;

struct {
	long int levelid;
	int levelversion;
	std::vector<DeathLocationMin> deaths;
} playingLevel;

std::optional<DeathsAnalyser> analyser = std::optional<DeathsAnalyser>();

struct Fields {
	EventListener<web::WebTask> m_listener;
};

// FUNCTIONS

void listDeathsOfID(long int levelid) {
	std::vector<DeathLocationMin> list;
	std::string url = API_BASE + "list";
	// TODO: fetch stuff
	// GET /list
	// write into playingLevel.deaths
};

void analyseDeathsOfID(long int levelid) {
	std::vector<DeathLocation> list;
	std::string url = API_BASE + "analyse";
	// TODO: fetch stuff
	// GET /analyse
	// analyser.value().deaths;
};

void postDeath(DeathLocationOut const& deathLoc, long int levelid) {
	bool sharingEnabled = Mod::get()->getSettingValue<bool>("share-deaths");
	if (!sharingEnabled) return;

	playingLevel.deaths.push_back((DeathLocationMin) deathLoc);

	// Initiate Listener
	EventListener<web::WebTask> listener;

	listener.bind([](web::WebTask::Event* e) {
		if (web::WebResponse* res = e->getValue()) {
			if (!res->ok())
				log::info(
					"Posting Death failed: {}",
					res->string().unwrapOr("Body could not be read.")
				);
		}
		else if (e->isCancelled()) {
			log::info("The request was cancelled... So sad :(");
		};
	});

	// Build the HTTP Request
	web::WebRequest req = web::WebRequest();

	auto myjson = matjson::Value();
	myjson.set("levelid", matjson::Value(playingLevel.levelid));
	myjson.set("playername", matjson::Value(playerData.username));
	myjson.set("accountid", matjson::Value(playerData.accountid));
	myjson.set("x", matjson::Value(deathLoc.x));

	req.bodyJSON(myjson);

	req.userAgent(HTTP_AGENT);

	req.header("Content-Type", "application/json");
	req.header("Accept", "application/json");

	req.timeout(std::chrono::seconds(10));

	std::string url = API_BASE + "submit";
	log::info("{}", url);
	//auto task = req.post(url);
	//listener.setFilter(task);

	// TODO: fetch stuff
	// POST /submit
};

// MODIFY UI

#include <Geode/modify/MenuLayer.hpp>
class $modify(MyMenuLayer, MenuLayer) {

	bool init() {

		if (!MenuLayer::init()) {
			return false;
		}

		auto myButton = CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("miniSkull_001.png"), this,
			menu_selector(MyMenuLayer::onMyButton)
		);

		auto menu = this->getChildByID("bottom-menu");
		menu->addChild(myButton);
		myButton->setID("my-button"_spr);

		menu->updateLayout(true);

		return true;
	}

	void onMyButton(CCObject*) {
		// Fires a DeathEvent
		log::info("{}", HTTP_AGENT);
	}
}
;

#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
class $modify(DeathMarkersPlayLayer, PlayLayer) {

	bool init(GJGameLevel * level, bool useReplay, bool dontCreateObjects) {

		// level->getCamera for figuring out what to draw
		// playingLevel.levelid = level->m_levelID;
		// playingLevel.levelversion = level->m_levelVersion;

		//log::info(level->isPlatformer() ? "platformer" : "classic");

		if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
			return false;
		}

		// Listen to deaths in here? (and then fire the global event)

		return true;
	};

	void destroyPlayer(PlayerObject * player, GameObject * object) override {
		log::info("haha i hooked your death");

		PlayLayer::destroyPlayer(player, object);

		//postDeath()
	};
}
;