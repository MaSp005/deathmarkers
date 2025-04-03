#pragma once
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;
using namespace std;

namespace dm {

	auto const HTTP_AGENT =
	"Geode-DeathMarkers-" + Mod::get()->getVersion().toVString(true);
	auto const FORMAT_VERSION = 1;

	// If you fork this mod and change the code, you should change this to the localhost
	// std::string const API_BASE = "http://localhost:8048/";
	std::string const API_BASE = "https://deathmarkers.masp005.dev/";
	auto const HTTP_TIMEOUT = std::chrono::seconds(15);

	struct playerData {
		std::string username = "";
		long userid = 0;
	};

	struct playingLevel {
		long levelId = 0;
		int levelversion = 0;
		bool practice = false;
		bool platformer = false;
		bool testmode = false;
	};

	// CLASSES

	// Holds only information about a death location that the server sends for regular gameplay
	class DeathLocationMin {
	public:
		CCPoint pos;
		int percentage;

		DeathLocationMin(float x, float y, int percentage);
		DeathLocationMin(CCPoint pos, int percentage);
		DeathLocationMin(float x, float y);
		DeathLocationMin(CCPoint pos);

		CCNode* createNode(bool isCurrent) const;
		CCNode* createAnimatedNode(bool isCurrent, double delay) const;
		CCNode* createNode(bool isCurrent, bool preAnim) const;
	};

	// Holds all information about a death location that can be sent to the server
	// Excludes info about the level, as it does not affect the death itself
	class DeathLocationOut : public DeathLocationMin {
	public:
		/*
		bool coin1 = false;
		bool coin2 = false;
		bool coin3 = false;
		int itemdata = 0;
		*/
		DeathLocationOut(float x, float y);
		DeathLocationOut(CCPoint pos);

		void addToJSON(matjson::Value* json) const;
	};

	// Holds all information about a death location that the server sends for analysis
	// Includes info about the level, because the server sends it for each individual death
	class DeathLocation : public DeathLocationMin {
	public:
		std::string userIdent;
		int levelVersion = 1;
		bool practice = false;
		bool clustered = false;
		/*
		bool coin1 = false;
		bool coin2 = false;
		bool coin3 = false;
		int itemdata = 0;
		*/
		CCSprite* node = nullptr;

		DeathLocation(float x, float y);
		DeathLocation(CCPoint pos);

		CCSprite* createNode();
		void updateNode();
	};

	void parseDeathList(web::WebResponse* res,
		vector<DeathLocationMin>* target);
	void parseDeathList(web::WebResponse* res,
		vector<DeathLocation>* target);

	vector<std::string> split(const std::string& string, const char at);

}