#include "editor.hpp"


#include <Geode/modify/LevelEditorLayer.hpp>
class $modify(DMEditorLayer, LevelEditorLayer) {

	struct Fields {
		EventListener<web::WebTask> m_listener;
		CCNode* m_dmNode = CCNode::create();
		std::vector<DeathLocation> m_deaths;
		bool enabled = false;
		bool loaded = false;
	};

	void fetch() {

		log::info("Listing Deaths...");
		this->m_fields->loaded = true;
		this->m_fields->m_deaths.clear();

		long levelId = this->m_level->m_levelID;
		if (levelId == 0) return;

		// Parse result JSON and add all as DeathLocationMin instances to playingLevel.deaths
		m_fields->m_listener.bind([this](web::WebTask::Event* const e) {
			auto res = e->getValue();
			if (res) {
				if (!res->ok())
					log::error("Listing Deaths failed: {}", res->string().unwrapOr("Body could not be read."));
				else {
					log::info("Received death list.");

					parseDeathList(res, &this->m_fields->m_deaths);

					analyzeData();
				}
			}
			else if (e->isCancelled()) {
				log::error("Death Listing Request was cancelled.");
			};
			});

		// Build the HTTP Request
		std::string const url = API_BASE + "analysis";
		web::WebRequest req = web::WebRequest();

		req.param("levelId", levelId);
		req.userAgent(HTTP_AGENT);
		req.timeout(HTTP_TIMEOUT);

		this->m_fields->m_listener.setFilter(req.get(url));

	}

	void toggleDeathMarkers() {

		if (!this->m_fields->enabled) {
			this->m_fields->enabled = true;

			if (!this->m_fields->loaded) fetch();
			else startUI();
		} else {
			this->m_fields->enabled = false;

			// Disable stuff

		}

	}

	void analyzeData() {
		// TODO: Parse and analyze data

		startUI();
	}

	void startUI() {

		this->m_fields->m_dmNode->setID("markers"_spr);
		this->m_fields->m_dmNode->setZOrder(2 << 28); // everyone using 99999 smh
		this->m_objectLayer->addChild(this->m_fields->m_dmNode);

		for (auto& deathLoc : this->m_fields->m_deaths) {
			auto node = deathLoc.toMin().createAnimatedNode(false, (static_cast<float>(rand()) / RAND_MAX) * .25f);
			this->m_fields->m_dmNode->addChild(node);
			if (deathLoc.percentage >= 0 && deathLoc.percentage <= 100);
		}

	}

};

#include <Geode/modify/EditorPauseLayer.hpp>
class $modify(DMEditorPauseLayer, EditorPauseLayer) {

	bool init(LevelEditorLayer * layer) {

		if (!EditorPauseLayer::init(layer)) return false;

		auto btn = CCMenuItemSpriteExtra::create(
			CCSprite::createWithSpriteFrameName("marker-button.png"_spr),
			this,
			menu_selector(DMEditorPauseLayer::onButton)
		);
		btn->setID("load-button"_spr);
		this->getChildByID("guidelines-menu")->addChild(btn);

		return true;

	}

	void onButton(CCObject * sender) {

		static_cast<DMEditorLayer*>(this->m_editorLayer)->fetch();

	}
};