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
		sprite->setScale(markerScale * 1.5);
		sprite->setZOrder(2 << 29);
	}
	else {
		sprite->setScale(markerScale);
		sprite->setZOrder(2 << 29 - 1);
	}
	if (preAnim) {
		auto point = CCPoint(this->pos.x, this->pos.y + markerScale * 4);
		sprite->setPosition(point);
		sprite->setOpacity(0);
	}
	else {
		sprite->setPosition(this->pos);
	}
	sprite->setZOrder(2 << 28 - !isCurrent);
	sprite->setAnchorPoint({ 0.5f, 0.0f });
	return sprite;
}


DeathLocationOut::DeathLocationOut(CCPoint pos) {
	this->pos = CCPoint(pos);
}

DeathLocationMin DeathLocationOut::toMin() const {
	return DeathLocationMin(this->pos, percentage);
}

void DeathLocationOut::addToJSON(matjson::Value* json) const {
	json->set("x", matjson::Value(this->pos.x));
	json->set("y", matjson::Value(this->pos.y));
	json->set("percentage", matjson::Value(this->percentage));
	json->set("coins", matjson::Value(this->coin1 | this->coin2 << 1 | this->coin3 << 2));
	json->set("itemdata", matjson::Value(this->itemdata));
}


DeathLocation::DeathLocation(float x, float y) {
	this->pos = CCPoint(x, y);
}

DeathLocation::DeathLocation(CCPoint pos) {
	this->pos = CCPoint(pos);
}

DeathLocationMin DeathLocation::toMin() const {
	return DeathLocationMin(this->pos, this->percentage);
}


void parseDeathList(web::WebResponse* res, std::vector<DeathLocationMin>* target) {

	auto const body = res->json();
	if (body.isErr())
		return log::error("Error reading list response: Body could not be read.");

	auto const okObj = body.ok();
	matjson::Value const parsed = okObj.value();
	if (!parsed.isArray())
		return log::error("Unexpected Non-Array response listing deaths: {}", parsed.dump(matjson::NO_INDENTATION));

	auto const& list = parsed.asArray().unwrap(); // [[#,#,#],[#,#,#],...]
	for (int i = 0; i < list.size(); i++) {
		auto const& item = list.at(i);
		if (!item.isArray())
			return log::error("Unexpected Non-Array item listing deaths: {}", item.dump(matjson::NO_INDENTATION));

		auto const& coords = item.asArray().unwrap(); // [#,#,#]
		if (coords.size() < 2 || coords.size() > 3)
			return log::error("Unexpected Non-2-or-3-Tuple listing deaths: {}, size {}", item.dump(matjson::NO_INDENTATION), coords.size());
		
		auto const& x = coords.at(0);
		auto const& y = coords.at(1);
		if (!x.isNumber() || !y.isNumber())
			return log::error("Unexpected Non-Number coordinate listing deaths: {}", item.dump(matjson::NO_INDENTATION));
		
		auto deathLoc = DeathLocationMin(x.asDouble().unwrap(), y.asDouble().unwrap());

		if (coords.size() == 3) {
			auto const& percentage = coords.at(2);
			if (!percentage.isNumber())
				return log::error("Unexpected Non-Number percentage listing deaths: {}", item.dump(matjson::NO_INDENTATION));
			deathLoc.percentage = percentage.asInt().unwrap();
		}

		target->push_back(deathLoc);
	}

}

void parseDeathList(web::WebResponse* res, std::vector<DeathLocation>* target) {

	auto const body = res->json();
	if (body.isErr())
		return log::error("Error reading list response: Body could not be read.");

	auto const okObj = body.ok();
	matjson::Value const parsed = okObj.value();
	if (!parsed.isArray())
		return log::error("Unexpected Non-Array response listing deaths: {}", parsed.dump(matjson::NO_INDENTATION));

	auto const& list = parsed.asArray().unwrap(); // [{...},{...},...]
	for (int i = 0; i < list.size(); i++) {
		auto const& item = list.at(i);
		if (!item.isObject()) {
			log::error("Unexpected Non-Object item listing deaths: {}", item.dump(matjson::NO_INDENTATION));
			continue;
		}

		auto const& useridentProp = item.get("userident");
		auto const& versionProp = item.get("levelversion");
		auto const& practiceProp = item.get("percentage");
		auto const& xProp = item.get("x");
		auto const& yProp = item.get("y");
		auto const& percentageProp = item.get("percentage");
		if (useridentProp.isErr() || versionProp.isErr() || practiceProp.isErr() ||
			xProp.isErr() || yProp.isErr() || percentageProp.isErr()) {
			log::error("Nonextistant property listing deaths: {}", item.dump(matjson::NO_INDENTATION));
			continue;
		}


		auto const& userident = useridentProp.unwrap();
		auto const& version = versionProp.unwrap();
		auto const& practice = practiceProp.unwrap();
		auto const& x = xProp.unwrap();
		auto const& y = yProp.unwrap();
		auto const& percentage = percentageProp.unwrap();
		if (!userident.isString() || !version.isNumber() || !practice.isNumber() ||
			!x.isNumber() || !y.isNumber() || !percentage.isNumber()) {
			log::error("Incorrect property type listing deaths: {}", item.dump(matjson::NO_INDENTATION));
			continue;
		}

		auto deathLoc = DeathLocation(x.asDouble().unwrap(), y.asDouble().unwrap());
		deathLoc.userIdent = userident.asString().unwrap();
		deathLoc.levelVersion = version.asInt().unwrap();
		deathLoc.practice = practice.asInt().unwrap() == 1;
		deathLoc.percentage = percentage.asInt().unwrap();

		target->push_back(deathLoc);
	}

}