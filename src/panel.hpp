#pragma once
#include <math.h>
#include "shared.hpp"

constexpr auto PANEL_ID = "panel"_spr;

class DMControlPanel : public CCLayer {

protected:
	CCSize m_size;
	CCDrawNode* m_drawNode = nullptr;
	bool setup(CCPoint const& anchor);

public:
	static DMControlPanel* create();

	void clear();
	
};