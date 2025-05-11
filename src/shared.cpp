#include "shared.hpp"

using namespace dm;

DeathLocationMin::DeathLocationMin(float x, float y, int percentage) {
	this->pos = CCPoint(x, y);
	this->percentage = percentage;
}

DeathLocationMin::DeathLocationMin(CCPoint pos, int percentage) {
	this->pos = CCPoint(pos);
	this->percentage = percentage;
}

DeathLocationMin::DeathLocationMin(float x, float y) {
	this->pos = CCPoint(x, y);
	this->percentage = 0;
}

DeathLocationMin::DeathLocationMin(CCPoint pos) {
	this->pos = CCPoint(pos);
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
	if (isCurrent) {
		sprite->setScale(markerScale * 1.5f);
		sprite->setZOrder(2 << 29);
	}
	else {
		sprite->setScale(markerScale);
		sprite->setZOrder((2 << 29) - 1);
	}
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

// Comparison based on x-position for common sorting (invoked by std::sort by default)
bool DeathLocationMin::operator<(const DeathLocationMin& other) const {
	return (this->pos.x < other.pos.x);
};


DeathLocationOut::DeathLocationOut(float x, float y) :
	DeathLocationMin::DeathLocationMin(x, y) {}

DeathLocationOut::DeathLocationOut(CCPoint pos) :
	DeathLocationMin::DeathLocationMin(pos) {}

void DeathLocationOut::addToJSON(matjson::Value* json) const {
	json->set("x", matjson::Value(this->pos.x));
	json->set("y", matjson::Value(this->pos.y));
	json->set("percentage", matjson::Value(this->percentage));
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


std::string uint8_to_hex_string(const uint8_t *v, const size_t s) {
  std::stringstream ss;

  ss << std::hex << std::setfill('0');

  for (int i = 0; i < s; i++) {
    ss << std::hex << std::setw(2) << static_cast<int>(v[i]);
  }

  return ss.str();
}


std::optional<DeathLocationMin> readCSVLine(std::string buffer, bool hasPercentage) {
	if (buffer.empty()) return std::nullopt;

	vector<std::string> coords = split(buffer, ',');
	if (coords.size() != (hasPercentage ? 3 : 2)) {
		log::warn(
			"Error listing deaths: Invalid number of elements: {} ({})",
			coords, hasPercentage
		);
		return std::nullopt;
	}

	// Reserve variables, extract strings
	float x;
	float y;
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

	auto deathLoc = DeathLocationMin(x, y);

	// If applicable, extract and interpret percentage
	if (hasPercentage) {
		int percent;
		auto const& percentStr = coords.at(2);
		try {
			percent = stoi(percentStr);
		} catch (invalid_argument) {
			log::warn(
				"Unexpected Non-Number coordinate listing deaths: {}",
				coords
			);
			return std::nullopt;
		}
		deathLoc.percentage = percent;
	}

	return std::optional<DeathLocationMin>(deathLoc);
}

vector<DeathLocationMin> dm::getLocalDeaths(int levelId, bool hasPercentage) {
	filesystem::path filePath = Mod::get()->getSaveDir() / numToString(levelId);
	vector<DeathLocationMin> deaths;
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
			if (deathLoc.has_value()) deaths.push_back(deathLoc.value());

			buffer = "";
		} else buffer += single;
	}
	auto deathLoc = readCSVLine(buffer, hasPercentage);
	if (deathLoc.has_value()) deaths.push_back(deathLoc.value());
	stream.close();
	return deaths;
}

void dm::storeLocalDeaths(int levelId, vector<DeathLocationMin>& deaths,
	bool hasPercentage) {
	filesystem::path filePath = Mod::get()->getSaveDir() / numToString(levelId);
	
	auto stream = ofstream(filePath);
	for (auto i = deaths.begin(); i < deaths.end(); i++) {
		stream << i->pos.x << "," << i->pos.y;
		if (hasPercentage) stream << "," << i->percentage;
		stream << "\n";
	}
};


void dm::parseBinDeathList(web::WebResponse* res,
	vector<DeathLocationMin>* target, bool hasPercentage) {

	auto const body = res->data();
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
		log::warn("{} exccess bytes, probably data misalignment! Skipping...",
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

		auto deathLoc = DeathLocationMin(stencil.obj.x, stencil.obj.y);
		deathLoc.percentage = stencil.obj.perc;
		target->push_back(deathLoc);
	}

}

void dm::parseBinDeathList(web::WebResponse* res,
	vector<DeathLocation>* target) {

	auto const body = res->data();
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
		log::warn("{} exccess bytes, probably data misalignment! Skipping...",
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


vector<DeathLocationMin>::iterator dm::binarySearchNearestXPosOnScreen(
	vector<DeathLocationMin>::iterator from,
	vector<DeathLocationMin>::iterator to, CCLayer* parent, float x) {

	if (to - from <= 1) return from;

	auto middle = from + ((to - from) >> 1);

	auto dist = parent->convertToWorldSpace(middle->pos).x;
	if (dist > x) to = middle;
	else from = middle;

	return binarySearchNearestXPosOnScreen(from, to, parent, x);

}