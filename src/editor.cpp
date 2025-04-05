#include "editor.hpp"

using namespace dm;

#include <Geode/modify/LevelEditorLayer.hpp>
class $modify(DMEditorLayer, LevelEditorLayer) {

	struct Fields {
		EventListener<web::WebTask> m_listener;

		CCNode* m_stackNode = nullptr;
		CCNode* m_dmNode = nullptr;
		CCMenuItemSprite* m_button = nullptr;

		vector<DeathLocation> m_deaths;

		bool m_enabled = false;
		bool m_loaded = false;
		bool m_showedGuide = false;
		float m_lastZoom = 0;
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
		m_fields->m_listener.bind(
			[this](web::WebTask::Event* const e) {
				auto res = e->getValue();
				if (res) {
					if (!res->ok()) {
						log::error("Listing Deaths failed: {}", res->string()
							.unwrapOr("Body could not be read."));

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
						parseBinDeathList(res, &this->m_fields->m_deaths);
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
			}
		);

		// Build the HTTP Request
		std::string const url = API_BASE + "analysis";
		web::WebRequest req = web::WebRequest();

		req.param("levelid", levelId);
		req.param("response", "bin");
		req.userAgent(HTTP_AGENT);
		req.timeout(HTTP_TIMEOUT);

		this->m_fields->m_listener.setFilter(req.get(url));

	}

	void toggleDeathMarkers() {

		if (!this->m_fields->m_enabled) {
			this->m_fields->m_enabled = true;

			if (!this->m_fields->m_loaded) {
				this->m_fields->m_button->setEnabled(false),
				fetch();
			}
			else startUI();
		}
		else {
			this->m_fields->m_enabled = false;
			this->m_fields->m_button->unselected();

			this->m_fields->m_dmNode->removeAllChildrenWithCleanup(true);
			this->m_fields->m_dmNode->removeFromParent();

			this->m_fields->m_stackNode->removeAllChildrenWithCleanup(true);
			this->m_fields->m_stackNode->removeFromParent();

			for (auto& deathLoc : this->m_fields->m_deaths)
				deathLoc.node = nullptr;

			this->unschedule(schedule_selector(DMEditorLayer::updateMarkers));
		}

	}

	void updateStacks(float maxDistance) {

		if (!Mod::get()->getSettingValue<bool>("stacks-in-editor")) return;

		auto deathStacks = new vector<DeathLocationStack>();

		identifyClusters(&this->m_fields->m_deaths, maxDistance, deathStacks);

		this->m_fields->m_stackNode->removeAllChildrenWithCleanup(true);
		for (auto stack = deathStacks->begin(); stack < deathStacks->end(); stack++) {
			// TODO: separate Texture for zero-size stack
			auto sprite = CCSprite::create("marker-group.png"_spr);
			sprite->setID("marker-stack"_spr);
			sprite->setScale(max(stack->circle.r * 2.125f, maxDistance / 2) / sprite->getContentWidth());
			sprite->setZOrder(1);
			sprite->setPosition(stack->circle.c);
			sprite->setAnchorPoint({ 0.5f, 0.5f });

			auto countText = CCLabelBMFont::create(
				numToString(stack->deaths.size(), 0).c_str(),
				"goldFont.fnt"
			);
			countText->setAnchorPoint({ 0.5f, 0.5f });
			countText->setPosition({
				sprite->getContentWidth() / 2,
				sprite->getContentHeight() / 2
			});
			sprite->addChild(countText);

			this->m_fields->m_stackNode->addChild(sprite);
		}

	}

	void analyzeData() {

		auto completed = set<std::string>();

		if (!this->m_level->isPlatformer()) {
			erase_if(this->m_fields->m_deaths,
				[&completed](const DeathLocation& death) {
					if (death.percentage != 101) return false;
					completed.insert(death.userIdent);
					return true;
				}
			);
		}

		if (false) {
			stable_sort(
				this->m_fields->m_deaths.begin(),
				this->m_fields->m_deaths.end(),
				[](DeathLocation a, DeathLocation b) {
					return a.userIdent.compare(b.userIdent) < 0;
				}
			);

			// TODO: analyze data
		}

		completed.clear();

		// Sort Deaths list along x-axis for better positional searching performance
		sort(
			this->m_fields->m_deaths.begin(),
			this->m_fields->m_deaths.end(),
			[](DeathLocation a, DeathLocation b) {
				return (a.pos.x < b.pos.x);
			}
		);

	}

	void startUI() {
		
		this->m_fields->m_button->setEnabled(true);
		this->m_fields->m_button->selected();

		if (!this->m_fields->m_showedGuide) {

			geode::createQuickPopup(
				"DeathMarkers",
				to_string(this->m_fields->m_deaths.size()) + " deaths were found.",
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
		this->m_fields->m_dmNode->setZOrder(-3);

		this->m_fields->m_stackNode = CCNode::create();
		this->m_fields->m_stackNode->setID("stacks"_spr);
		this->m_fields->m_stackNode->setZOrder(-2);

		for (auto& deathLoc : this->m_fields->m_deaths) {
			auto node = deathLoc.createNode();
			node->setZOrder(0);
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
		darkNode->setZOrder(-1);
		darkNode->setPosition(CCPoint(0, 0));
		darkNode->setAnchorPoint(CCPoint(0, 0));
		this->m_editorUI->addChild(darkNode);
		*/

		this->m_editorUI->addChild(this->m_fields->m_dmNode);
		this->m_editorUI->addChild(this->m_fields->m_stackNode);
		this->schedule(schedule_selector(DMEditorLayer::updateMarkers), 0);
		
		updateStacks(20 / this->m_objectLayer->getScale());

	}

	void updateMarkers(float) {
		this->m_fields->m_dmNode->setPosition(this->m_objectLayer->getPosition());
		this->m_fields->m_dmNode->setScale(this->m_objectLayer->getScale());
		this->m_fields->m_stackNode->setPosition(this->m_objectLayer->getPosition());
		this->m_fields->m_stackNode->setScale(this->m_objectLayer->getScale());

		// Counters UI zoom, keeps markers at constant size relative to screen
		float inverseScale = Mod::get()->getSettingValue<float>("marker-scale") /
			this->m_objectLayer->getScale();

		if (this->m_fields->m_lastZoom != this->m_objectLayer->getScale()) {
			updateStacks(20 / this->m_objectLayer->getScale());
			this->m_fields->m_lastZoom = this->m_objectLayer->getScale();
		}

		CCArray* children = this->m_fields->m_dmNode->getChildren();
		for (int i = 0; i < this->m_fields->m_dmNode->getChildrenCount(); i++) {
			auto child = static_cast<CCNode*>(children->objectAtIndex(i));
			child->setScale(inverseScale / 2);
		}
		
		for (auto& deathLoc : this->m_fields->m_deaths) {
			deathLoc.updateNode();
		}
	}

};

#include <Geode/modify/EditorPauseLayer.hpp>
class $modify(DMEditorPauseLayer, EditorPauseLayer) {

	bool init(LevelEditorLayer * layer) {

		if (!EditorPauseLayer::init(layer)) return false;

		auto editor = static_cast<DMEditorLayer*>(this->m_editorLayer);

		auto offSprite = CCSprite::create("marker-button-deact.png"_spr);

		if (!editor->m_fields->m_button) {
			editor->m_fields->m_button = CCMenuItemExt::createSprite(
				offSprite,
				CCSprite::create("marker-button-on.png"_spr),
				LoadingSpinner::create(offSprite->getContentWidth()),
				[editor](auto el) {
					editor->toggleDeathMarkers();
				}
			);
			editor->m_fields->m_button->setID("load-button"_spr);
		}

		auto menu = this->getChildByID("guidelines-menu");
		menu->addChild(editor->m_fields->m_button);
		menu->updateLayout(true);

		return true;

	}
};