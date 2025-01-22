#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

auto const HTTP_AGENT =
"Geode-DeathMarkers-" + Mod::get()->getVersion().toVString(true);
auto const FORMAT_VERSION = 1;

// If you fork this mod and change the code, you should change this to the localhost
std::string const API_BASE = "http://localhost:8048/";
//std::string const API_BASE = "https://deathmarkers.masp005.dev/";
auto const HTTP_TIMEOUT = std::chrono::seconds(15);

struct playerData {
	std::string username = "";
	long int userid = 0;
};

struct playingLevel {
	long int levelid = 0;
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

// FYI i too would like to inherit DeathLocationMin here but c++ is messing around with it

// Holds all information about a death location that can be sent to the server
class DeathLocationOut {
public:
	CCPoint pos;
	int percentage = 0;
	bool coin1 = false;
	bool coin2 = false;
	bool coin3 = false;
	int itemdata = 0;

	DeathLocationOut(CCPoint pos);

	DeathLocationMin toMin() const;

	void addToJSON(matjson::Value* json) const;
};

// Holds all information about a death location that the server sends for analysis
class DeathLocation {
public:
	std::string userIdent;
	CCPoint pos;
	int percentage = 0;
	bool coin1 = false;
	bool coin2 = false;
	bool coin3 = false;
	int itemdata = 0;

	DeathLocation(float x, float y);
	DeathLocation(CCPoint pos);

	DeathLocationMin toMin() const;
};