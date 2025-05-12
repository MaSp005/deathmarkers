#include "editor.hpp"

void DMEditorLayer::fetch() {

	log::info("Listing Deaths...");
	this->m_fields->m_loaded = true;
	this->m_fields->m_deaths.clear();

	int levelId = this->m_level->m_levelID;
	if (levelId == 0) levelId = this->m_level->m_originalLevel;
	if (levelId == 0) {
		FLAlertLayer::create(
			"DeathMarkers",
			"This is neither a published nor a copied level. Deaths are not obtainable for this level.",
			"OK"
		)->show();
		this->m_fields->m_enabled = false;
		this->m_fields->m_loaded = false;
		if (auto button = this->getChildByIDRecursive(BUTTON_ID)) {
			static_cast<CCMenuItemSprite*>(button)->unselected();
		}
		return;
	}

	if (auto button = this->getChildByIDRecursive(BUTTON_ID)) {
		auto menuEl = static_cast<CCMenuItemSprite*>(button);
		menuEl->unselected();
		menuEl->setEnabled(false);
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
					if (auto button = this->getChildByIDRecursive(BUTTON_ID)) {
						auto menuEl = static_cast<CCMenuItemSprite*>(button);
						menuEl->setEnabled(true);
						menuEl->unselected();
					}
				}
				else {
					log::debug("Received death list.");
					parseBinDeathList(res, &this->m_fields->m_deaths);
					log::debug("Finished parsing.");
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
				if (auto button = this->getChildByIDRecursive(BUTTON_ID)) {
					auto menuEl = static_cast<CCMenuItemSprite*>(button);
					menuEl->setEnabled(true);
					menuEl->unselected();
				}
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

void DMEditorLayer::toggleDeathMarkers() {

	if (!this->m_fields->m_enabled) {
		this->m_fields->m_enabled = true;

		if (!this->m_fields->m_loaded) {
			fetch();
		} else startUI();
	} else {
		this->m_fields->m_enabled = false;
		if (auto button = this->getChildByIDRecursive(BUTTON_ID)) {
			static_cast<CCMenuItemSprite*>(button)->unselected();
		}

		if (auto panel = this->m_editorUI->getChildByID(PANEL_ID))
			panel->removeFromParent();

		this->m_fields->m_dmNode->removeAllChildrenWithCleanup(true);
		this->m_fields->m_dmNode->removeFromParent();

		this->m_fields->m_stackNode->removeAllChildrenWithCleanup(true);
		this->m_fields->m_stackNode->removeFromParent();

		this->m_fields->m_darkNode->removeFromParent();

		for (auto& deathLoc : this->m_fields->m_deaths)
			deathLoc.node = nullptr;

		this->unschedule(schedule_selector(DMEditorLayer::updateMarkers));
	}

}

void DMEditorLayer::updateStacks(float maxDistance) {

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

void DMEditorLayer::analyzeData() {

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
			[](const DeathLocation& a, const DeathLocation& b) {
				return a.userIdent.compare(b.userIdent) < 0;
			}
		);

		std::string currIdent = "";
		for (auto i = this->m_fields->m_deaths.begin(); i < this->m_fields->m_deaths.end(); i++) {
			if (i->userIdent == currIdent) {
				i->previous = i - 1;
				i->previous->next = i;
			} else currIdent = i->userIdent;
		}
		// TODO: analyze data
	}

	completed.clear();

	// Sort Deaths list along x-axis for better positional searching performance during clustering
	sort(
		this->m_fields->m_deaths.begin(),
		this->m_fields->m_deaths.end()
	);

}

void DMEditorLayer::startUI() {
		
	if (auto button = this->getChildByIDRecursive(BUTTON_ID)) {
		auto menuEl = static_cast<CCMenuItemSprite*>(button);
		menuEl->setEnabled(true);
		menuEl->selected();
	}

	if (!this->m_fields->m_showedGuide) {

		geode::createQuickPopup(
			"DeathMarkers",
			fmt::format("{} deaths were found.", this->m_fields->m_deaths.size()),
			"Continue", "Open Guide",
			[](auto, bool open) {
				if (open) web::openLinkInBrowser(API_BASE);
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

	this->m_fields->m_panel = DMControlPanel::create();
	this->m_editorUI->addChild(this->m_fields->m_panel);

	for (auto& deathLoc : this->m_fields->m_deaths) {
		auto node = deathLoc.createNode();
		node->setZOrder(0);
		this->m_fields->m_dmNode->addChild(node);
	}

	if (!this->m_fields->m_darkNode) {
		auto winSize = CCDirector::sharedDirector()->getWinSize();

		this->m_fields->m_darkNode =
			CCSprite::createWithSpriteFrameName("lightsquare_01_01_color_001.png");
		this->m_fields->m_darkNode->setID("darkener"_spr);
		auto& spriteSize = this->m_fields->m_darkNode->getContentSize();
		this->m_fields->m_darkNode->setScale(
			winSize.width / spriteSize.width,
			winSize.height / spriteSize.height
		);
		this->m_fields->m_darkNode->setPosition(CCPoint(0, 0));
		this->m_fields->m_darkNode->setAnchorPoint(CCPoint(0, 0));
		this->m_fields->m_darkNode->setColor({0, 0, 0});
		this->m_fields->m_darkNode->setOpacity(
			Mod::get()->getSettingValue<int64_t>("darken-editor"));
		this->m_fields->m_darkNode->setZOrder(-4);
	}

	this->m_editorUI->addChild(this->m_fields->m_darkNode);

	this->m_editorUI->addChild(this->m_fields->m_dmNode);
	this->m_editorUI->addChild(this->m_fields->m_stackNode);
	this->schedule(schedule_selector(DMEditorLayer::updateMarkers), 0);
		
	updateStacks(20 / this->m_objectLayer->getScale());

}

void DMEditorLayer::updateMarkers(float) {
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


bool DMEditorPauseLayer::init(LevelEditorLayer * layer) {

	if (!EditorPauseLayer::init(layer)) return false;

	auto editor = static_cast<DMEditorLayer*>(layer);
	auto offSprite = CCSprite::create("marker-button-deact.png"_spr);
	auto onSprite = CCSprite::create("marker-button-on.png"_spr);
	this->m_fields->m_button = CCMenuItemExt::createSprite(
		offSprite,
		onSprite,
		LoadingSpinner::create(offSprite->getContentWidth()),
		[editor](auto el) {
			editor->toggleDeathMarkers();
		}
	);
	this->m_fields->m_button->setID(BUTTON_ID);
	editor->m_fields->m_enabled ?
		this->m_fields->m_button->selected() :
		this->m_fields->m_button->unselected();

	auto menu = this->getChildByID("guidelines-menu");
	menu->addChild(this->m_fields->m_button);
	menu->updateLayout(true);

	return true;

}
