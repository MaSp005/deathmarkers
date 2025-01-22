#include "shared.hpp"

class DeathLocationMin {
public:
	CCPoint pos;
	int percentage;

	DeathLocationMin(float x, float y, int percentage) {
		this->pos = CCPoint(x, y);
		this->percentage = percentage;
	}

	DeathLocationMin(CCPoint pos, int percentage) {
		this->pos = CCPoint(pos);
		this->percentage = percentage;
	}

	DeathLocationMin(float x, float y) {
		this->pos = CCPoint(x, y);
		this->percentage = 0;
	}

	DeathLocationMin(CCPoint pos) {
		this->pos = CCPoint(pos);
		this->percentage = 0;
	}

	CCNode* createNode(bool isCurrent) const {
		return this->createNode(isCurrent, false);
	}

	CCNode* createAnimatedNode(bool isCurrent, double delay) const {
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

	CCNode* createNode(bool isCurrent, bool preAnim) const {
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
};

class DeathLocationOut {
public:
	CCPoint pos;
	int percentage = 0;
	bool coin1 = false;
	bool coin2 = false;
	bool coin3 = false;
	int itemdata = 0;

	DeathLocationOut(CCPoint pos) {
		this->pos = CCPoint(pos);
	}

	DeathLocationMin toMin() const {
		return DeathLocationMin(this->pos, percentage);
	}

	void addToJSON(matjson::Value* json) const {
		json->set("x", matjson::Value(this->pos.x));
		json->set("y", matjson::Value(this->pos.y));
		json->set("percentage", matjson::Value(this->percentage));
		json->set("coins", matjson::Value(this->coin1 | this->coin2 << 1 | this->coin3 << 2));
		json->set("itemdata", matjson::Value(this->itemdata));
	}
};

class DeathLocation {
public:
	std::string userIdent;
	CCPoint pos;
	int percentage = 0;
	bool coin1 = false;
	bool coin2 = false;
	bool coin3 = false;
	int itemdata = 0;

	DeathLocation(float x, float y) {
		this->pos = CCPoint(x, y);
	}

	DeathLocation(CCPoint pos) {
		this->pos = CCPoint(pos);
	}

	DeathLocationMin toMin() const {
		return DeathLocationMin(this->pos, this->percentage);
	}
};