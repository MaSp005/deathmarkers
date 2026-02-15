#pragma once
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <ctime>

using namespace geode::prelude;
using namespace std;
using geode::utils::web::WebResponse;

namespace dm {

	auto const HTTP_AGENT =
	"Geode-DeathMarkers-" + Mod::get()->getVersion().toVString(true);
	auto const FORMAT_VERSION = 1;

	auto const HTTP_TIMEOUT = std::chrono::seconds(15);
	
	constexpr int CURRENT_ZORDER = 2 << 29;
	constexpr int OTHER_ZORDER = (2 << 29) - 1;
	constexpr float GS_WEIGHT_RED = 0.299;
	constexpr float GS_WEIGHT_GREEN = 0.587;
	constexpr float GS_WEIGHT_BLUE = 0.114;

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
		virtual CCNode* createAnimatedNode(bool isCurrent, double delay, double fadeTime) const;
		virtual CCNode* createNode(bool isCurrent, bool preAnim) const;
		virtual void printCSV(ostream& os, bool hasPercentage) const;

		bool operator<(const DeathLocationMin& other) const;
	};

	// Dynamic replacement for DeathLocationMin when using Ghost Cubes
	// Stores additional needed information and overrides rendering
	class GhostLocation : public DeathLocationMin {
	public:
		float rotation;
		IconType mode;
		bool isPlayer2;
		bool isMini;
		bool isFlipped;
		bool isMirrored;
		static inline bool shouldUse = false;

		GhostLocation(PlayerObject* player);
		GhostLocation(float x, float y);
		GhostLocation(float x, float y, int percentage);
		
		CCNode* createAnimatedNode(bool isCurrent, double delay, double fadeTime)
			const override;
		CCNode* createNode(bool isCurrent, bool preAnim) const override;
		void printCSV(ostream& os, bool hasPercentage) const override;
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
		bool practice = false;
		std::time_t realTime = 0;
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

	struct LocationComparer {
    bool operator()(const DeathLocationMin& a,
                    const DeathLocationMin& b) const {
        return a.pos.x < b.pos.x;
    }
	};

	struct LocationComparerPtr {
    bool operator()(const std::unique_ptr<DeathLocationMin>& a,
                    const std::unique_ptr<DeathLocationMin>& b) const {
        return a->pos.x < b->pos.x;
    }
	};

	bool shouldSubmit(struct playingLevel& level, struct playerData& player);
	bool willEverDraw(struct playingLevel& level);

	std::string makeRequestURL(char const* endpoint);
	ccColor3B grayscale(ccColor3B const& color);

	vector<unique_ptr<DeathLocationMin>> getLocalDeaths(int levelId,
		bool hasPercentage);
	void storeLocalDeaths(int levelId,
		vector<unique_ptr<DeathLocationMin>> const& deaths, bool hasPercentage);

	void parseBinDeathList(WebResponse res,
		vector<unique_ptr<DeathLocationMin>>* target, bool hasPercentage);
	void parseBinDeathList(WebResponse res,
		vector<DeathLocation>* target);

	vector<std::string> split(const std::string& string, const char at);

	vector<unique_ptr<DeathLocationMin>>::iterator binarySearchNearestXPosOnScreen(
		vector<unique_ptr<DeathLocationMin>>::iterator from,
		vector<unique_ptr<DeathLocationMin>>::iterator to, CCLayer* parent, float x,
		bool preferHigher);

	vector<unique_ptr<DeathLocationMin>>::iterator binarySearchNearestXPos(
		vector<unique_ptr<DeathLocationMin>>::iterator from,
		vector<unique_ptr<DeathLocationMin>>::iterator to, float x,
		bool preferHigher);

}

// for whatever fucking reason, listenForClose is protected, so this is a bypass class
class PopupBypass : public geode::Popup {
public:
	// PopupBypass() : geode::Popup() {};

	static geode::Popup::CloseEvent listenForCloseOn(geode::Popup* popup) {
		return static_cast<PopupBypass*>(popup)->listenForCloseBypass();
	};

	geode::Popup::CloseEvent listenForCloseBypass() {
		return this->listenForClose();
	}
};
