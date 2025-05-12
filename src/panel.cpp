#include "panel.hpp"

DMControlPanel* DMControlPanel::create() {
	//if (instance) return instance;
  auto ret = new DMControlPanel();
  auto winSize = cocos2d::CCDirector::get()->getWinSize();
  if (ret->setup(
    CCPoint(winSize.width - 230, winSize.height - 130)
  )) {
    //ret->autorelease();
    //return instance = ret;
    return ret;
  }

  delete ret;
  return nullptr;
}

bool DMControlPanel::setup(CCPoint const& anchor) {
	this->setID(PANEL_ID);

	m_size = CCSize(120, 80);
	this->setContentSize(m_size);
	this->setPosition(anchor);

  auto bgSprite = extension::CCScale9Sprite::create(
    "geode.loader/GE_square01.png",
    { 0, 0, 80, 80 }
  );
  bgSprite->setContentSize(m_size);
  bgSprite->setPosition(m_size / 2);
	bgSprite->setOpacity(128);
	this->addChild(bgSprite);

	auto title = CCLabelBMFont::create("DeathMarkers Control Panel", "goldFont.fnt");
	title->setAnchorPoint(CCPoint(0.f, 0.5f));
	title->setScale((m_size.width - 20) / title->getContentWidth());
	title->setPosition(5, m_size.height + 2.78f);
	this->addChild(title);

  m_drawNode = CCDrawNode::create();
  m_drawNode->setAnchorPoint(CCPoint(0.5f, 0.5f));
  m_drawNode->setContentSize(m_size);
  m_drawNode->setPosition(m_size / 2);
  this->addChild(m_drawNode);

  clear();

  return true;
}

void DMControlPanel::clear() {
  m_drawNode->clear();
}
