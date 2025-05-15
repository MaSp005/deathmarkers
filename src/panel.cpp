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


float mod(float a, float b) {
  float r = a - static_cast<int>(a / b) * b;
  return r < 0 ? r + b : r;
}

CCPoint angleFrom(CCPoint const& center, float angle, float radius) {
  return center + CCPoint(
    sin(angle) * radius,
    cos(angle) * radius
  );
}


float constexpr circlePointInterval = M_PI / 90;

void DMControlPanel::drawPartialCircle(CCPoint const& center,
  float from, float to, float radius, ccColor4F const& color) {

  from = mod(from, 2 * M_PI);
  to = from + mod(to - from, 2 * M_PI);
  if (from == to) return;

  int const pointCount = static_cast<int>
    (ceil((to - from) / circlePointInterval));
  
  CCPoint verts[static_cast<int>(M_PI * 2 / circlePointInterval) + 2];
  verts[0] = center;
  int i = 0;
  for (; i < pointCount; i++) {
    verts[i + 1] = angleFrom(center, from + i * circlePointInterval, radius);
  }
  verts[++i] = angleFrom(center, to, radius);
  m_drawNode->drawPolygon(verts, i + 1, color, 0.f, {});

  for (int j = 0; j < i + 1; j++) {
    log::debug("{}: {}", j, verts[j]);
  }
}

void DMControlPanel::drawPieGraph(vector<struct dataSlice> slices) {

  auto center = CCPoint(m_size.height / 2, m_size.height / 2);
  float radius = center.x * 0.8f;

  if (slices.size() == 0) return;
  if (slices.size() == 1) {
    m_drawNode->drawDot(center, radius, slices[0].color);
    return;
  }

  float total = 0.f;
  for (auto slice = slices.begin(); slice < slices.end(); slice++) {
    if (slice->quantity <= 0) {
      slices.erase(slice);
      slice--;
      continue;
    }
    total += slice->quantity;
  }
  if (!total) return;
  
  float lastAngle = 0.f;

  for (auto& slice : slices) {
    auto endAngle = lastAngle + static_cast<float>((slice.quantity / total) * 2 * M_PI);
    drawPartialCircle(
      center, 
      lastAngle, 
      endAngle, 
      radius, 
      slice.color
    );
    log::debug("{} -> {} ({} / {}) {}", lastAngle, endAngle, slice.quantity, total, slice.color);
    lastAngle = endAngle;
  }
}

void DMControlPanel::drawBarGraph(vector<struct dataSlice> slices, float y = 30.f, float height = 20.f) {

  float width = this->m_size.width * 0.8f;

  float total = 0.f;
  for (auto slice = slices.begin(); slice < slices.end(); slice++) {
    if (slice->quantity <= 0) {
      slices.erase(slice);
      slice--;
      continue;
    }
    total += slice->quantity;
  }
  if (!total) return;

  float left = this->m_size.width * 0.1f;

  for (auto& slice : slices) {
    float sliceWidth = slice.quantity / total * width;
    this->m_drawNode->drawRect(
      CCRect {
        left, y,
        sliceWidth, height
      },
      slice.color, 0.f, slice.color
    );
    left += sliceWidth;
  }
}
