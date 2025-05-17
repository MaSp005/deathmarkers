#pragma once
#include <math.h>
#include "shared.hpp"

constexpr auto PANEL_ID = "panel"_spr;

struct dataSlice {
	float quantity;
	ccColor4F color;
	std::string label;
};

struct partialCircle {
	float angle;
	ccColor4F color;
};

class DMControlPanel : public CCLayer {

protected:
	CCSize m_size;
	CCDrawNode* m_drawNode = nullptr;
	bool setup(CCPoint const& anchor);
	void drawPartialCircle(
		CCPoint const& center,
		float from, 
		float to, 
		float radius, 
		ccColor4F const& color
	);

public:
	static DMControlPanel* create();

	void clear();
	void drawPieGraph(
		vector<struct dataSlice> slices
	);
	void drawBarGraph(vector<struct dataSlice> slices, float y = 30.f, float height = 20.f);
	
};