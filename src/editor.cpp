#include "editor.hpp"

#include <Geode/modify/LevelEditorLayer.hpp>
class $modify(DMEditorLayer, LevelEditorLayer) {

	struct Fields {
		EventListener<web::WebTask> m_listener;
		CCNode* m_dmNode = nullptr;
		std::vector<DeathLocation> m_deaths;
		bool m_enabled = false;
		bool m_loaded = false;
		bool m_showedGuide = false;
		CCMenuItemSprite* m_button = nullptr;
	};

	void fetch() {

		log::info("Listing Deaths...");
		this->m_fields->m_loaded = true;
		this->m_fields->m_deaths.clear();

		long levelId = this->m_level->m_levelID;
		if (levelId == 0) levelId = this->m_level->m_originalLevel;
		if (levelId == 0) {
			FLAlertLayer::create(
				"DeathMarkers",
				"This is neither a published nor a copied level. Deaths are not obtainable for this level.",
				"OK"
			)->show();
			this->m_fields->m_enabled = false;
			this->m_fields->m_loaded = false;
			return;
		}

		// Parse result JSON and add all as DeathLocationMin instances to playingLevel.deaths
		m_fields->m_listener.bind([this](web::WebTask::Event* const e) {
			auto res = e->getValue();
			if (res) {
				if (!res->ok()) {
					log::error("Listing Deaths failed: {}", res->string().unwrapOr("Body could not be read."));

					FLAlertLayer::create(
						"DeathMarkers",
						"Deaths could not be fetched. Please try again later.",
						"OK"
					)->show();
					this->m_fields->m_enabled = false;
					this->m_fields->m_loaded = false;
				}
				else {
					log::info("Received death list.");
					parseDeathList(res, &this->m_fields->m_deaths);
					log::info("Finished parsing.");
					analyzeData();
					startUI();
				}
			}
			else if (e->isCancelled()) {
				log::error("Death Listing Request was cancelled.");

				FLAlertLayer::create(
					"DeathMarkers",
					"Deaths could not be fetched. Please try again later.",
					"OK"
				)->show();
				this->m_fields->m_enabled = false;
				this->m_fields->m_loaded = false;
			};
			});

		// Build the HTTP Request
		std::string const url = API_BASE + "analysis";
		web::WebRequest req = web::WebRequest();

		req.param("levelid", levelId);
		req.param("response", "csv");
		req.userAgent(HTTP_AGENT);
		req.timeout(HTTP_TIMEOUT);

		this->m_fields->m_listener.setFilter(req.get(url));

	}

	void toggleDeathMarkers() {

		if (!this->m_fields->m_enabled) {
			this->m_fields->m_enabled = true;
			this->m_fields->m_button->selected();

			if (!this->m_fields->m_loaded) fetch();
			else startUI();
		}
		else {
			this->m_fields->m_enabled = false;
			this->m_fields->m_button->unselected();

			this->m_fields->m_dmNode->removeAllChildrenWithCleanup(true);
			this->m_fields->m_dmNode->removeFromParent();

			this->unschedule(schedule_selector(DMEditorLayer::myUpdate));

		}

	}

	void analyzeData() {
		// TODO: Parse and analyze data

	}

	void startUI() {

		if (!this->m_fields->m_showedGuide) {

			geode::createQuickPopup(
				"DeathMarkers",
				std::to_string(this->m_fields->m_deaths.size()) + " deaths were found.",
				"Continue", "Open Guide",
				[](auto, bool open) {
					if (!open) return;
					web::openLinkInBrowser(API_BASE);
				}
			);
			this->m_fields->m_showedGuide = true;

		}

		this->m_fields->m_dmNode = CCNode::create();
		this->m_fields->m_dmNode->setID("markers"_spr);
		this->m_fields->m_dmNode->setZOrder(2 << 28); // everyone using 99999 smh

		// TODO: Display stacked deaths, multiple in close proximity
		for (int i = 0; i < this->m_fields->m_deaths.size(); i++) {
			auto& deathLoc = this->m_fields->m_deaths.at(i);
			auto node = deathLoc.toMin().createNode(false);
			node->setZOrder(1);
			this->m_fields->m_dmNode->addChild(node);
		}

		// TODO: Fix darkener node
		/*
		auto winSize = CCDirector::sharedDirector()->getWinSize();
		auto darkNode = CCNodeRGBA::create();
		darkNode->setID("darkener"_spr);
		darkNode->setContentSize(winSize);
		darkNode->setColor(ccColor3B(0, 0, 0));
		darkNode->setOpacity(128);
		darkNode->setZOrder(0);
		darkNode->setPosition(CCPoint(0, 0));
		darkNode->setAnchorPoint(CCPoint(0, 0));
		this->m_editorUI->addChild(darkNode);
		*/

		this->m_editorUI->addChild(this->m_fields->m_dmNode);
		this->schedule(schedule_selector(DMEditorLayer::myUpdate), 0);

	}

	void myUpdate(float) {
		this->m_fields->m_dmNode->setPosition(this->m_objectLayer->getPosition());
		this->m_fields->m_dmNode->setScale(this->m_objectLayer->getScale());

		// Counters UI zoom, keeps markers at constant size relative to screen
		float inverseScale = Mod::get()->getSettingValue<float>("marker-scale") / this->m_objectLayer->getScale();

		CCArray* children = this->m_fields->m_dmNode->getChildren();
		for (int i = 0; i < this->m_fields->m_dmNode->getChildrenCount(); i++) {
			auto child = static_cast<CCNode*>(children->objectAtIndex(i));
			child->setScale(inverseScale);
		}
	}

};

#include <Geode/modify/EditorPauseLayer.hpp>
class $modify(DMEditorPauseLayer, EditorPauseLayer) {

	bool init(LevelEditorLayer * layer) {

		if (!EditorPauseLayer::init(layer)) return false;

		auto editor = static_cast<DMEditorLayer*>(this->m_editorLayer);

		if (!editor->m_fields->m_button) {
			auto btn = CCMenuItemExt::createSprite(
				CCSprite::create("marker-button-deact.png"_spr),
				CCSprite::create("marker-button-on.png"_spr),
				[editor](auto el) {
					editor->toggleDeathMarkers();
				}
			);
			btn->setID("load-button"_spr);

			editor->m_fields->m_button = btn;
		}
		this->getChildByID("guidelines-menu")->addChild(editor->m_fields->m_button);

		return true;

	}
};