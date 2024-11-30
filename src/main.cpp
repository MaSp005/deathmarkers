/**
 * Include the Geode headers.
 */
#include <Geode/Geode.hpp>
#include <vector>
#include <string>

using namespace geode::prelude;

#define DEATH_MARKERS_API_BASE "https://deathmarkers.masp005.dev/"

class DeathEvent : public Event {};

class DeathLocationMin {
	double x;
	double y;
};

class DeathLocationOut : public DeathLocationMin {
	int percentage;
	bool coin1;
	bool coin2;
	bool coin3;
	int itemdata;
};

class DeathLocation : public DeathLocationOut {
	std::string userIdent;
	int percentage;
	bool coin1;
	bool coin2;
	bool coin3;
	int itemdata;
};

struct {
	std::string username;
	long int accountid;
} playerData;

// GET /list
std::vector<DeathLocationMin> listDeathsOfID(long int levelid) {
	std::vector<DeathLocationMin> list;
	std::string url = *DEATH_MARKERS_API_BASE + "list";
	// TODO: fetch stuff
	return list;
};

// GET /analyse
std::vector<DeathLocation> analyseDeathsOfID(long int levelid) {
	std::vector<DeathLocation> list;
	std::string url = *DEATH_MARKERS_API_BASE + "analyse";
	// TODO: fetch stuff
	return list;
};

// GET /submit
bool postDeath(DeathLocationOut deathLoc, long int levelid) {
	std::string url = *DEATH_MARKERS_API_BASE + "submit";
	// TODO: fetch stuff
	return true;
};

/**
 * `$modify` lets you extend and modify GD's classes.
 * To hook a function in Geode, simply $modify the class
 * and write a new function definition with the signature of
 * the function you want to hook.
 *
 * Here we use the overloaded `$modify` macro to set our own class name,
 * so that we can use it for button callbacks.
 *
 * Notice the header being included, you *must* include the header for
 * the class you are modifying, or you will get a compile error.
 */
#include <Geode/modify/MenuLayer.hpp>
class $modify(MyMenuLayer, MenuLayer) {
	/**
	 * Typically classes in GD are initialized using the `init` function,
	 * (though not always!), so here we use it to add our own button to the
	 * bottom menu.
	 *
	 * Note that for all hooks, your signature has to *match exactly*,
	 * `void init()` would not place a hook!
	 */
	bool init() {
		/**
		 * We call the original init function so that the
		 * original class is properly initialized.
		 */
		if (!MenuLayer::init()) {
			return false;
		}

		/**
		 * See this page for more info about buttons
		 * https://docs.geode-sdk.org/tutorials/buttons
		 */
		auto myButton = CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("miniSkull_001.png"), this,
			/**
			 * Here we use the name we set earlier for our modify class.
			 */
			menu_selector(MyMenuLayer::onMyButton));

		/**
		 * Here we access the `bottom-menu` node by its ID, and add our button to it.
		 * Node IDs are a Geode feature, see this page for more info about it:
		 * https://docs.geode-sdk.org/tutorials/nodetree
		 */
		auto menu = this->getChildByID("bottom-menu");
		menu->addChild(myButton);

		/**
		 * The `_spr` string literal operator just prefixes the string with
		 * your mod id followed by a slash. This is good practice for setting your own
		 * node ids.
		 */
		myButton->setID("my-button"_spr);

		/**
		 * We update the layout of the menu to ensure that our button properly
		 * placed. This is yet another Geode feature, see this page more info about
		 * it: https://docs.geode-sdk.org/tutorials/layouts
		 */
		menu->updateLayout(true);

		/**
		 * We return `true` to indicate that the class was properly
		initialized.
		 */
		return true;
	}

	/**
	 * This is the callback function for the button we created earlier.
	 * The signature for button callbacks must always be the same,
	 * return type `void` and taking a `CCObject*`.
	 */
	void onMyButton(CCObject*) {
		DeathEvent().post();
	}
}
;

$execute {

	new EventListener<EventFilter<DeathEvent>>(+[](DeathEvent* ev) {
		log::info("911 whos this");

		listDeathsOfID(9823493);
		// Propagate event
		return ListenerResult::Propagate;
	});

}