#include "shared.hpp"

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

CCNode* DeathLocationMin::createAnimatedNode(bool isCurrent, double delay) const {
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


void DeathLocationOut::addToJSON(matjson::Value* json) const {
	json->set("x", matjson::Value(this->pos.x));
	json->set("y", matjson::Value(this->pos.y));
	json->set("percentage", matjson::Value(this->percentage));
	//json->set("coins", matjson::Value(this->coin1 | this->coin2 << 1 | this->coin3 << 2));
	//json->set("itemdata", matjson::Value(this->itemdata));
}


DeathLocation::DeathLocation(float x, float y) : DeathLocationMin::DeathLocationMin(x, y) {}

DeathLocation::DeathLocation(CCPoint pos) : DeathLocationMin::DeathLocationMin(pos) {}

CCNode* DeathLocation::createNode() const {
	auto sprite = CCSprite::create("death-marker.png"_spr);
	std::string const id = "marker"_spr;
	float markerScale = Mod::get()->getSettingValue<float>("marker-scale");
	sprite->setScale(markerScale);
	sprite->setPosition(this->pos);
	sprite->setAnchorPoint({ 0.5f, 0.0f });
	// TODO: fix clustered attribute
	// if (this->clustered) {
	//     sprite->setOpacity(128);
	//     sprite->setScale(markerScale / 2);
	// }
	return sprite;
}


void parseDeathList(web::WebResponse* res, std::vector<DeathLocationMin>* target) {

	auto const body = res->string();

	if (body.isErr())
		return log::error("Error reading list response: Body could not be read.");

	// Read and split up body
	gd::string okObj = body.ok().value();
	if (okObj.empty()) return log::info("Responded with zero deaths.");

	auto lines = split(&okObj, '\n');
	auto header = split(&lines.at(0), ',');
	lines.erase(lines.begin());

	// Identify columns
	int xIdx = std::find(header.begin(), header.end(), "x") - header.begin();
	int yIdx = std::find(header.begin(), header.end(), "y") - header.begin();
	int percentIdx = std::find(header.begin(), header.end(), "percentage") - header.begin();
	if (xIdx == -1 || yIdx == -1) return log::warn("Property not featured in header: {}", header);

	// Iterate lines
	for (int i = 0; i < lines.size(); i++) {
		auto const& line = lines.at(i);
		if (line.empty()) return;

		auto coords = split(&line, ',');
		if (coords.size() != header.size()) {
			log::warn("Error listing deaths: Inequal number of elements: {} | {}", header, coords);
			continue;
		}

		// Reserve variables, extract strings
		float x;
		float y;
		auto const& xStr = coords.at(xIdx);
		auto const& yStr = coords.at(yIdx);

		// Interpret Strings into Numbers
		try {
			x = std::stof(xStr);
			y = std::stof(yStr);
		} catch (std::invalid_argument) {
			log::warn("Unexpected Non-Number coordinate listing deaths: {}", coords);
			continue;
		}

		auto deathLoc = DeathLocationMin(x, y);

		// If applicable, extract and interpret percentage
		if (percentIdx != -1) {
			int percent;
			auto const& percentStr = coords.at(percentIdx);
			try {
				percent = std::stoi(percentStr);
			} catch (std::invalid_argument) {
				log::warn("Unexpected Non-Number coordinate listing deaths: {}", coords);
				continue;
			}
			deathLoc.percentage = percent;
		}

		target->push_back(deathLoc);
	}

}

void parseDeathList(web::WebResponse* res, std::vector<DeathLocation>* target) {

	auto const body = res->string();

	if (body.isErr())
		return log::error("Error reading list response: Body could not be read.");

	// Read and split up body
	gd::string okObj = body.ok().value();
	auto lines = split(&okObj, '\n');
	auto header = split(&lines.at(0), ',');
	lines.erase(lines.begin());

	// Identify columns
	int useridentIdx = std::find(header.begin(), header.end(), "userident") - header.begin();
	int versionIdx = std::find(header.begin(), header.end(), "levelversion") - header.begin();
	int practiceIdx = std::find(header.begin(), header.end(), "practice") - header.begin();
	int xIdx = std::find(header.begin(), header.end(), "x") - header.begin();
	int yIdx = std::find(header.begin(), header.end(), "y") - header.begin();
	int percentageIdx = std::find(header.begin(), header.end(), "percentage") - header.begin();
	if (useridentIdx == -1 || versionIdx == -1 || practiceIdx == -1 || xIdx == -1 || yIdx == -1 || percentageIdx == -1)
		return log::warn("Property not featured in header: {}", header);

	// Iterate lines
	for (int i = 0; i < lines.size(); i++) {
		auto const& line = lines.at(i);
		if (line.empty()) return;

		auto coords = split(&line, ',');
		if (coords.size() != header.size()) {
			log::warn("Error analyzing deaths: Inequal number of elements: {} | {}", header, coords);
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
			levelVersion = std::stoi(versionStr);
			x = std::stof(xStr);
			y = std::stof(yStr);
			percent = std::stoi(percentStr);
		}
		catch (std::invalid_argument) {
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

std::vector<gd::string> split(const gd::string* string, const char at) {
	auto result = std::vector<gd::string>();
	int currentStart = 0;

	while (true) {
		int nextSplit = string->find_first_of(at, currentStart);
		if (nextSplit == std::string::npos) {
			result.push_back(string->substr(currentStart, string->size() - currentStart));
			return result;
		}
		int nextLength = nextSplit - currentStart;
		result.push_back(string->substr(currentStart, nextLength));
		currentStart = nextSplit + 1;
	}
}