#include "shared.hpp"

using namespace dm;

DeathLocationMin::DeathLocationMin(float x, float y, int percentage) {
	this->pos = CCPoint(x, y);
	this->percentage = percentage;
}

DeathLocationMin::DeathLocationMin(CCPoint pos, int percentage) {
	this->pos = pos;
	this->percentage = percentage;
}

DeathLocationMin::DeathLocationMin(float x, float y) {
	this->pos = CCPoint(x, y);
	this->percentage = 0;
}

DeathLocationMin::DeathLocationMin(CCPoint pos) {
	this->pos = pos;
	this->percentage = 0;
}

CCNode* DeathLocationMin::createNode(bool isCurrent) const {
	return this->createNode(isCurrent, false);
}

CCNode* DeathLocationMin::createAnimatedNode(
	bool isCurrent, double delay, double fadeTime) const {
	auto node = this->createNode(isCurrent, true);
	if (delay || fadeTime)
		node->runAction(CCSequence::createWithTwoActions(
			CCDelayTime::create(delay),
			CCSpawn::createWithTwoActions(
				CCEaseBounceOut::create(
					CCMoveTo::create(fadeTime, this->pos)
				),
				CCFadeIn::create(fadeTime)
			)
		));
	return node;
}

CCNode* DeathLocationMin::createNode(bool isCurrent, bool preAnim) const {
	auto sprite = CCSprite::create("death-marker.png"_spr);
	float markerScale = Mod::get()->getSettingValue<float>("marker-scale");

	sprite->setZOrder(isCurrent ? CURRENT_ZORDER : OTHER_ZORDER);

	if (preAnim) {
		auto point = CCPoint(this->pos.x, this->pos.y + markerScale * 4);
		sprite->setPosition(point);
		sprite->setOpacity(0);
	}
	else {
		sprite->setPosition(this->pos);
	}
	sprite->setAnchorPoint({ 0.5f, 0.0f });
	return sprite;
}

void DeathLocationMin::printCSV(ostream& os, bool hasPercentage) const {
	os << this->pos.x << ',' << this->pos.y;
	if (hasPercentage) os << ',' << this->percentage;
};


GhostLocation::GhostLocation(PlayerObject* player) :
	DeathLocationMin(player->getPosition()) {
	this->isPlayer2 = player->m_isSecondPlayer;
	this->isMini = player->m_vehicleSize == 0.6f;
	this->isFlipped = player->m_isUpsideDown;
	this->isMirrored = player->m_isGoingLeft;
	this->rotation = player->m_fRotationX;

	if (player->m_isShip) {
		this->mode = player->m_isPlatformer ? IconType::Jetpack : IconType::Ship;
	} else if (player->m_isBall) {
		this->mode = IconType::Ball;
	} else if (player->m_isBird) {
		this->mode = IconType::Ufo;
	} else if (player->m_isDart) {
		this->mode = IconType::Wave;
	} else if (player->m_isRobot) {
		this->mode = IconType::Robot;
	} else if (player->m_isSpider) {
		this->mode = IconType::Spider;
	} else if (player->m_isSwing) {
		this->mode = IconType::Swing;
	} else {
		this->mode = IconType::Cube;
	}
}

GhostLocation::GhostLocation(float x, float y) :
	DeathLocationMin(x, y) {};

GhostLocation::GhostLocation(float x, float y, int percentage) :
	DeathLocationMin(x, y, percentage) {};

CCNode* GhostLocation::createAnimatedNode(
	bool isCurrent, double delay, double fadeTime) const {

	if (!GhostLocation::shouldUse)
		return DeathLocationMin::createAnimatedNode(isCurrent, delay, fadeTime);

	auto node = this->createNode(isCurrent, true);
	if (delay || fadeTime)
		node->runAction(CCSequence::createWithTwoActions(
			CCDelayTime::create(delay),
			CCSpawn::createWithTwoActions(
				CCEaseBounceOut::create(
					CCScaleTo::create(fadeTime, isMini ? 0.6f : 1.0f)
				),
				CCFadeTo::create(fadeTime, 0xff / 2)
			)
		));
	return node;
}

CCNode* GhostLocation::createNode(bool isCurrent, bool preAnim) const {

	if (!GhostLocation::shouldUse)
		return DeathLocationMin::createNode(isCurrent, preAnim);

	auto gm = GameManager::sharedState();

	int frameIcon;
	switch (this->mode) {
		case IconType::Ship: frameIcon = gm->getPlayerShip(); break;
		case IconType::Ball: frameIcon = gm->getPlayerBall(); break;
		case IconType::Ufo: frameIcon = gm->getPlayerBird(); break;
		case IconType::Wave: frameIcon = gm->getPlayerDart(); break;
		case IconType::Robot: frameIcon = gm->getPlayerRobot(); break;
		case IconType::Spider: frameIcon = gm->getPlayerSpider(); break;
		case IconType::Swing: frameIcon = gm->getPlayerSwing(); break;
		case IconType::Jetpack: frameIcon = gm->getPlayerJetpack(); break;
		default: frameIcon = gm->getPlayerFrame();
	}

	auto col1 = gm->getPlayerColor();
	auto col2 = gm->getPlayerColor2();
	auto glowOutline = gm->colorForIdx(gm->getPlayerGlowColor());
	if (this->isPlayer2) std::swap(col1, col2);

	SimplePlayer* sprite = SimplePlayer::create(0);
	if (this->mode == IconType::Ship || this->mode == IconType::Ufo ||
		this->mode == IconType::Jetpack
	) {
		auto miniPlayer = SimplePlayer::create(0);

		miniPlayer->updatePlayerFrame(gm->getPlayerFrame(), IconType::Cube);
		miniPlayer->setColors(
			grayscale(gm->colorForIdx(col1)),
			grayscale(gm->colorForIdx(col2))
		);
		miniPlayer->setGlowOutline(grayscale(glowOutline));
		miniPlayer->enableCustomGlowColor(glowOutline);
		if (!gm->getPlayerGlow())
			miniPlayer->disableGlowOutline();

		miniPlayer->setZOrder(-1);
		miniPlayer->setAnchorPoint({ 0.5f, 0.5f });
		miniPlayer->setOpacity(0xff / 2);

		switch (this->mode) {
			case IconType::Ship:
				miniPlayer->setPosition({0, 10});
				miniPlayer->setScale(0.55f);
				break;
			case IconType::Ufo:
				miniPlayer->setPosition({0, 5});
				miniPlayer->setScale(0.55f);
				break;
			case IconType::Jetpack:
				miniPlayer->setPosition({6, 4});
				miniPlayer->setScale(0.6f);
				break;
			// just to shut clang up
			default: break;
		}

		sprite->addChild(miniPlayer);
	}

	sprite->updatePlayerFrame(frameIcon, this->mode);
	sprite->setColors(
		grayscale(gm->colorForIdx(col1)),
		grayscale(gm->colorForIdx(col2))
	);
	sprite->setGlowOutline(grayscale(glowOutline));
	sprite->enableCustomGlowColor(glowOutline);
	if (!gm->getPlayerGlow())
		sprite->disableGlowOutline();

	sprite->setRotation(this->rotation);
	if (mode != IconType::Ball && mode != IconType::Swing) {
		if (this->isFlipped) sprite->m_fRotationX += 180.0f;
		if (this->isMirrored) sprite->m_fRotationY += 180.0f;
	}
	sprite->setScale(1.0f / (1 << (preAnim + isMini)));

	sprite->setOpacity(preAnim ? 0 : 0xff / 2);
	sprite->setCascadeOpacityEnabled(true);
	sprite->setPosition(pos);
	if (this->mode == IconType::Ship) sprite->setPosition(pos + CCPoint(0, -5));
	sprite->setZOrder(isCurrent ? CURRENT_ZORDER : OTHER_ZORDER);
	sprite->setAnchorPoint({ 0.5f, 0.5f });
	return sprite;
}

void GhostLocation::printCSV(ostream& os, bool hasPercentage) const {
	uint8_t flagField = 0;
	flagField |= (this->isPlayer2 << 0);
	flagField |= (this->isMini << 1);
	flagField |= (this->isFlipped << 2);
	flagField |= (this->isMirrored << 3);

	os << this->pos.x << ',' << this->pos.y
		 << ',' << this->rotation << ',' << static_cast<int>(this->mode) << ','
		 << static_cast<int>(flagField);
	if (hasPercentage) os << ',' << this->percentage;
};


DeathLocationOut::DeathLocationOut(float x, float y) :
	DeathLocationMin::DeathLocationMin(x, y) {
	this->realTime = std::time(nullptr);
}

DeathLocationOut::DeathLocationOut(CCPoint pos) :
	DeathLocationMin::DeathLocationMin(pos) {
	this->realTime = std::time(nullptr);
}

void DeathLocationOut::addToJSON(matjson::Value* json) const {
	json->set("x", matjson::Value(this->pos.x));
	json->set("y", matjson::Value(this->pos.y));
	json->set("percentage", matjson::Value(this->percentage));
	json->set("practice", matjson::Value(this->practice));
	//json->set("coins", matjson::Value(this->coin1 | this->coin2 << 1 | this->coin3 << 2));
	//json->set("itemdata", matjson::Value(this->itemdata));
}


DeathLocation::DeathLocation(float x, float y) :
	DeathLocationMin::DeathLocationMin(x, y) {}

DeathLocation::DeathLocation(CCPoint pos) :
	DeathLocationMin::DeathLocationMin(pos) {}

CCSprite* DeathLocation::createNode() {
	if (this->node) return this->node;

	this->node = CCSprite::create(); // Omit texture for caller to apply and change
	this->updateNode();

	std::string const id = "marker"_spr;
	float markerScale = Mod::get()->getSettingValue<float>("marker-scale");
	this->node->setScale(markerScale);
	this->node->setPosition(this->pos);
	this->node->setAnchorPoint({ 0.5f, 0.0f });
	return this->node;
}

void DeathLocation::updateNode() {
	this->node->initWithFile(this->clustered ? "mini-marker.png"_spr : "death-marker.png"_spr);
	this->node->setAnchorPoint({ 0.5f, 0.0f });
}


bool dm::shouldSubmit(struct playingLevel& level, struct playerData& player) {
	// Ignore Testmode and local Levels
	if (level.testmode) return false;
	if (level.levelId == 0) return false;

	// Respect User Setting
	auto sharingEnabled = Mod::get()->getSettingValue<bool>("share-deaths");
	if (!sharingEnabled) return false;

	return true;
};

bool dm::willEverDraw(struct playingLevel& level) {
	auto playLayer = PlayLayer::get();
	// auto mod = Mod::get();

	if (!playLayer) return false;
	if (level.levelId == 0) return false; // Don't draw on local levels

	// We can't check settings here since they can change whenever
	// LEGACY: Test settings here, refresh when changed, fetch late if needed
	// float scale = mod->getSettingValue<float>("marker-scale");
	// int histHeight = mod->getSettingValue<int>("prog-bar-hist-height");
	// if (scale == 0 && histHeight == 0) return false;

	// string normalCond = mod->getSettingValue<std::string>("condition-normal");
	// if (normalCond != "Never") return true;
	// string practiceCond = mod->getSettingValue<std::string>("condition-practice");
	// if (practiceCond != "Never") return true;

	// return mod->getSettingValue<bool>("show-in-pause");
	return true;
};


std::string dm::makeRequestURL(char const* endpoint) {
	auto mod = Mod::get();
	auto settValue = mod->getSettingValue<std::string>("server-url");

	// trim whitespace at the end (from https://stackoverflow.com/a/217605)
	auto endWhitespace = std::find_if(settValue.rbegin(), settValue.rend(),
		[](unsigned char ch) {
			return !std::isspace(ch);
		}).base();

	if (endWhitespace != settValue.end()) {
		settValue.erase(endWhitespace, settValue.end());
		mod->setSettingValue("server-url", settValue);
	}
	if (!settValue.ends_with("/")) {
		settValue.append("/");
		mod->setSettingValue("server-url", settValue);
	}
	return settValue + (endpoint[0] == '/' ? endpoint + 1 : endpoint);
}

std::string uint8_to_hex_string(const uint8_t *v, const size_t s) {
  std::stringstream ss;

  ss << std::hex << std::setfill('0');

  for (int i = 0; i < s; i++) {
    ss << std::hex << std::setw(2) << static_cast<int>(v[i]);
  }

  return ss.str();
}

ccColor3B dm::grayscale(ccColor3B const& color) {
	auto brightness = static_cast<uint8_t>(color.r * GS_WEIGHT_RED +
		color.r * GS_WEIGHT_RED + color.b * GS_WEIGHT_BLUE);
	return {brightness, brightness, brightness};
};


std::optional<unique_ptr<DeathLocationMin>> readCSVLine(
	std::string const& buffer, bool hasPercentage) {
	if (buffer.empty()) return std::nullopt;

	vector<std::string> coords = split(buffer, ',');
	bool isGhost = coords.size() > 3;
	int expect = (hasPercentage ? 3 : 2) + (isGhost * 3);
	if (coords.size() != expect) {
		log::warn(
			"Error listing deaths: Unexpected number of elements: {} ({})",
			coords, hasPercentage
		);
		return std::nullopt;
	}

	// Reserve variables, extract strings
	float x;
	float y;
	int percent;
	auto const& xStr = coords.at(0);
	auto const& yStr = coords.at(1);

	// Interpret Strings into Numbers
	try {
		x = stof(xStr);
		y = stof(yStr);
	} catch (invalid_argument) {
		log::warn("Unexpected Non-Number coordinate listing deaths: {}", coords);
		return std::nullopt;
	}

	// If applicable, extract and interpret percentage
	if (hasPercentage) {
		auto const& percentStr = coords.at(isGhost ? 5 : 2);
		try {
			percent = stoi(percentStr);
		} catch (invalid_argument) {
			log::warn(
				"Unexpected Non-Number coordinate listing deaths: {}",
				coords
			);
			return std::nullopt;
		}
	}

	if (isGhost) {
		auto ghostLoc = make_unique<GhostLocation>(GhostLocation(x, y, percent));

		float rotation;
		IconType mode;
		uint8_t flagField;
		auto const& rotStr = coords.at(2);
		auto const& modeStr = coords.at(3);
		auto const& flagStr = coords.at(4);

		try {
			rotation = stof(rotStr);
			mode = static_cast<IconType>(stof(modeStr));
			flagField = stoi(flagStr);
		} catch (invalid_argument) {
			log::warn("Unexpected Non-Number coordinate listing deaths: {}", coords);
			return std::nullopt;
		}

		ghostLoc->rotation = rotation;
		ghostLoc->mode = mode;
		ghostLoc->isPlayer2 = flagField & 1;
		ghostLoc->isMini = flagField & 2;
		ghostLoc->isFlipped = flagField & 4;
		ghostLoc->isMirrored = flagField & 8;

		return std::optional<unique_ptr<DeathLocationMin>>(std::move(ghostLoc));
	} else {
		auto deathLoc = make_unique<DeathLocationMin>(
			DeathLocationMin(x, y, percent)
		);

		return std::optional<unique_ptr<DeathLocationMin>>(std::move(deathLoc));
	}
}

vector<unique_ptr<DeathLocationMin>> dm::getLocalDeaths(int levelId, bool hasPercentage) {
	filesystem::path filePath = Mod::get()->getSaveDir() / numToString(levelId);
	vector<unique_ptr<DeathLocationMin>> deaths;
	if (!filesystem::exists(filePath)) {
		log::debug("No file found at {}.", filePath);
		return deaths;
	}

	auto stream = ifstream(filePath);
	std::string buffer;
	char single;
	while (stream.read(&single, 1)) {
		if (single == '\n') {
			// Process and clear buffer
			auto deathLoc = readCSVLine(buffer, hasPercentage);
			buffer = "";
			if (!deathLoc.has_value()) continue;
			deaths.push_back(std::move(*deathLoc));
		} else buffer += single;
	}
	auto deathLoc = readCSVLine(buffer, hasPercentage);
	if (deathLoc.has_value()) deaths.push_back(std::move(*deathLoc));
	stream.close();
	std::sort(
		deaths.begin(),
		deaths.end(),
		LocationComparerPtr{}
	);
	return deaths;
}

void dm::storeLocalDeaths(int levelId,
	vector<unique_ptr<DeathLocationMin>> const& deaths, bool hasPercentage) {
	filesystem::path filePath = Mod::get()->getSaveDir() / numToString(levelId);

	auto stream = ofstream(filePath);
	for (auto& i : deaths) {
		i->printCSV(stream, hasPercentage);
		stream << endl;
	}
};


void dm::parseBinDeathList(WebResponse res,
	vector<unique_ptr<DeathLocationMin>>* target, bool hasPercentage) {

	auto const body = res.data();
	int const elementWidth = 4 + 4 + (hasPercentage ? 2 : 0);
	if (body.size() <= elementWidth) return;
	uint8_t version = body[0];

	int const deathCount = body.size() / elementWidth;
	log::info(
		"Got {} bytes of info, segment width {} (has percentage: {}) -> versioning byte {:#02x} + {} deaths",
		body.size(), elementWidth, hasPercentage ? "y" : "n", version, deathCount
	);
	if (version != 1) {
		log::warn("Unknown version {}! Skipping...", version);
		return;
	}
	if ((body.size() - 1) % elementWidth) {
		log::warn("{} excess bytes, probably data misalignment! Skipping...",
			(body.size() - 1) % elementWidth);
		return;
	}
	target->reserve(deathCount);

	for (auto off = 1; off <= body.size() - elementWidth + 1; off += elementWidth) {
#pragma pack(push, 1)
		union stencil {
			struct dmObj {
				float x;
				float y;
				uint16_t perc;
			} obj;
			uint8_t raw[10];
		} stencil{};
#pragma pack(pop)

		std::memcpy(stencil.raw, body.data() + off, elementWidth);

		auto deathLoc = std::make_unique<DeathLocationMin>
			(DeathLocationMin(stencil.obj.x, stencil.obj.y));
		deathLoc->percentage = stencil.obj.perc;
		target->push_back(std::move(deathLoc));
	}

}

void dm::parseBinDeathList(WebResponse res,
	vector<DeathLocation>* target) {

	auto const body = res.data();
	int const elementWidth = 20 + 1 + 1 + 4 + 4 + 2;
	if (body.size() <= elementWidth) return;
	uint8_t version = body[0];

	int const deathCount = body.size() / elementWidth;
	log::info(
		"Got {} bytes of info, segment width {} -> versioning byte {:#02x} + {} deaths",
		body.size(), elementWidth, version, deathCount
	);
	if (version != 1) {
		log::warn("Unknown version {}! Skipping...", version);
		return;
	}
	if ((body.size() - 1) % elementWidth) {
		log::warn("{} excess bytes, probably data misalignment! Skipping...",
			(body.size() - 1) % elementWidth);
		return;
	}
	target->reserve(deathCount);

	for (auto off = 1; off <= body.size() - elementWidth + 1; off += elementWidth) {
#pragma pack(push, 1)
		union stencil {
			struct dmObj {
				uint8_t ident[20];
				uint8_t levelversion;
				uint8_t practice;
				float x;
				float y;
				uint16_t perc;
			} obj;
			uint8_t raw[32];
		} stencil{};
#pragma pack(pop)

		std::memcpy(stencil.raw, body.data() + off, elementWidth);

		auto deathLoc = DeathLocation(stencil.obj.x, stencil.obj.y);
		std::string userident = uint8_to_hex_string(stencil.obj.ident, 20);
		deathLoc.userIdent = userident;
		deathLoc.levelVersion = stencil.obj.levelversion;
		deathLoc.practice = stencil.obj.practice != 0;
		if (stencil.obj.practice > 1)
			log::warn("Practice attribute = {:x}, probable data misalignment!",
				stencil.obj.practice);
		deathLoc.percentage = stencil.obj.perc;
		target->push_back(deathLoc);
	}

}

vector<std::string> dm::split(const std::string& string, const char at) {
	auto result = vector<std::string>();
	int currentStart = 0;

	while (true) {
		int nextSplit = string.find_first_of(at, currentStart);
		if (nextSplit == std::string::npos) {
			result.push_back(string.substr(
				currentStart, string.size() - currentStart
			));
			return result;
		}
		int nextLength = nextSplit - currentStart;
		result.push_back(string.substr(currentStart, nextLength));
		currentStart = nextSplit + 1;
	}
}


vector<unique_ptr<DeathLocationMin>>::iterator dm::binarySearchNearestXPosOnScreen(
	vector<unique_ptr<DeathLocationMin>>::iterator from,
	vector<unique_ptr<DeathLocationMin>>::iterator to, CCLayer* parent, float x,
	bool preferHigher) {

	if (to - from <= 1) return preferHigher ? to : from;

	auto middle = from + ((to - from) / 2);

	auto dist = parent->convertToWorldSpace((*middle)->pos).x;
	if (dist > x) to = middle;
	else from = middle;

	return binarySearchNearestXPosOnScreen(from, to, parent, x, preferHigher);

}

vector<unique_ptr<DeathLocationMin>>::iterator dm::binarySearchNearestXPos(
	vector<unique_ptr<DeathLocationMin>>::iterator from,
	vector<unique_ptr<DeathLocationMin>>::iterator to, float x,
	bool preferHigher) {

	if (to - from <= 1) return preferHigher ? to : from;

	auto middle = from + ((to - from) / 2);

	if ((*middle)->pos.x > x) to = middle;
	else from = middle;

	return binarySearchNearestXPos(from, to, x, preferHigher);

}
