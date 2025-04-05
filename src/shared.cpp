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
	bool isCurrent, double delay) const {
	auto node = this->createNode(isCurrent, true);
	node->runAction(CCSequence::createWithTwoActions(
		CCDelayTime::create(delay),
		CCSpawn::createWithTwoActions(
			CCEaseBounceOut::create(
				CCMoveTo::create(0.25f, this->pos)
			),
			CCFadeIn::create(0.25f)
		)
	));
	return node;
}

CCNode* DeathLocationMin::createNode(bool isCurrent, bool preAnim) const {
	auto sprite = CCSprite::create("death-marker.png"_spr);
	std::string const id = "marker"_spr;
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


void dm::parseCsvDeathList(web::WebResponse* res,
	vector<DeathLocationMin>* target) {

	auto const body = res->string();

	if (body.isErr())
		return log::error("Error reading list response: Body could not be read.");

	// Read and split up body
	std::string const okObj = move(body.ok().value());
	if (okObj.empty()) return log::info("Responded with no data.");

	vector<std::string> lines = split(okObj, '\n');
	vector<std::string> header = split(lines.at(0), ',');
	lines.erase(lines.begin());

	// Identify columns
	int xIdx = find(header.begin(), header.end(), "x") - header.begin();
	int yIdx = find(header.begin(), header.end(), "y") - header.begin();
	int percentIdx = find(header.begin(), header.end(), "percentage") -
		header.begin();
	if (xIdx == -1 || yIdx == -1) return
		log::warn("Property not featured in header: {}", header);

	// Iterate lines
	for (int i = 0; i < lines.size(); i++) {
		auto const& line = lines.at(i);
		if (line.empty()) return;

		vector<std::string> coords = split(line, ',');
		if (coords.size() != header.size()) {
			log::warn(
				"Error listing deaths: Inequal number of elements: {} | {}",
				header, coords
			);
			continue;
		}

		// Reserve variables, extract strings
		float x;
		float y;
		auto const& xStr = coords.at(xIdx);
		auto const& yStr = coords.at(yIdx);

		// Interpret Strings into Numbers
		try {
			x = stof(xStr);
			y = stof(yStr);
		} catch (invalid_argument) {
			log::warn("Unexpected Non-Number coordinate listing deaths: {}", coords);
			continue;
		}

		auto deathLoc = DeathLocationMin(x, y);

		// If applicable, extract and interpret percentage
		if (percentIdx != -1) {
			int percent;
			auto const& percentStr = coords.at(percentIdx);
			try {
				percent = stoi(percentStr);
			} catch (invalid_argument) {
				log::warn(
					"Unexpected Non-Number coordinate listing deaths: {}",
					coords
				);
				continue;
			}
			deathLoc.percentage = percent;
		}

		target->push_back(deathLoc);
	}

}

void dm::parseCsvDeathList(web::WebResponse* res,
	vector<DeathLocation>* target) {

	auto const body = res->string();

	if (body.isErr())
		return log::error("Error reading list response: Body could not be read.");

	// Read and split up body
	std::string const okObj = move(body.ok().value());
	if (okObj.empty()) return log::info("Responded with no data.");

	vector<std::string> lines = split(okObj, '\n');
	vector<std::string> header = split(lines.at(0), ',');
	lines.erase(lines.begin());

	// Identify columns
	int useridentIdx = find(header.begin(), header.end(), "userident") -
		header.begin();
	int versionIdx = find(header.begin(), header.end(), "levelversion") -
		header.begin();
	int practiceIdx = find(header.begin(), header.end(), "practice") -
		header.begin();
	int xIdx = find(header.begin(), header.end(), "x") - header.begin();
	int yIdx = find(header.begin(), header.end(), "y") - header.begin();
	int percentageIdx = find(header.begin(), header.end(), "percentage") -
		header.begin();
	if (
		useridentIdx == -1 || versionIdx == -1 || practiceIdx == -1 ||
		xIdx == -1 || yIdx == -1 || percentageIdx == -1
	) return log::warn("Property not featured in header: {}", header);

	// Iterate lines
	for (int i = 0; i < lines.size(); i++) {
		auto const& line = lines.at(i);
		if (line.empty()) return;

		vector<std::string> coords = split(line, ',');
		if (coords.size() != header.size()) {
			log::warn(
				"Error analyzing deaths: Inequal number of elements: {} | {}",
				header, coords
			);
			continue;
		}

		// Reserve variables, extract strings
		gd::string userident = coords.at(useridentIdx);
		int levelVersion;
		bool practice = coords.at(practiceIdx) == "1";
		int percent;
		float x;
		float y;
		auto const& versionStr = coords.at(versionIdx);
		auto const& practiceStr = coords.at(practiceIdx);
		auto const& xStr = coords.at(xIdx);
		auto const& yStr = coords.at(yIdx);
		auto const& percentStr = coords.at(percentageIdx);

		// Interpret Strings into Numbers
		try {
			levelVersion = stoi(versionStr);
			x = stof(xStr);
			y = stof(yStr);
			percent = stoi(percentStr);
		}
		catch (invalid_argument) {
			log::warn("Unexpected Non-Number coordinate listing deaths: {}", line);
			continue;
		}

		// Populate DeathLocation Object
		auto deathLoc = DeathLocation(x, y);
		deathLoc.userIdent = userident;
		deathLoc.levelVersion = levelVersion;
		deathLoc.practice = practice;
		deathLoc.percentage = percent;

		target->push_back(deathLoc);
	}

}

void dm::parseBinDeathList(web::WebResponse* res,
	vector<DeathLocationMin>* target, bool hasPercentage) {

	auto const body = res->data();
	if (!body.size()) return;

	int const elementWidth = 4 + 4 + (hasPercentage ? 2 : 0);
	int const deathCount = body.size() / elementWidth;
	if (body.size() % elementWidth) {
		log::warn("{:x} exccess bytes, probably data misalignment! Skipping...",
			body.size() % elementWidth);
		return;
	}
	target->reserve(deathCount);

	log::info(
		"{} bytes of info, segWidth {}",
		body.size(), elementWidth
	);

	for (auto off = 0; off <= body.size() - elementWidth; off += elementWidth) {
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
	if (!body.size()) return;

	int const elementWidth = 20 + 1 + 1 + 4 + 4 + 2;
	int const deathCount = body.size() / elementWidth;
	if (body.size() % elementWidth) {
		log::warn("{:x} exccess bytes, probably data misalignment! Skipping...",
			body.size() % elementWidth);
		return;
	}
	target->reserve(deathCount);

	log::info(
		"{} bytes of info, segWidth {}",
		body.size(), elementWidth
	);

	for (auto off = 0; off <= body.size() - elementWidth; off += elementWidth) {
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
		log::info("gen userid: {}", userident);
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