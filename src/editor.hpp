#pragma once
#include <Geode/ui/LoadingSpinner.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <vector>
#include "shared.hpp"
#include "cluster.hpp"

using namespace dm;
constexpr auto BUTTON_ID = "load-button"_spr;
constexpr auto PANEL_ID = "panel"_spr;

#include <Geode/modify/LevelEditorLayer.hpp>
class DMEditorLayerDummy; struct DMEditorLayer : geode::Modify<DMEditorLayer, LevelEditorLayer> {

	struct Fields {
		EventListener<web::WebTask> m_listener;

		CCNode* m_stackNode = nullptr;
		CCNode* m_dmNode = nullptr;
		CCNodeRGBA* m_darkNode = nullptr;

		vector<DeathLocation> m_deaths;

		bool m_enabled = false;
		bool m_loaded = false;
		bool m_showedGuide = false;
		float m_lastZoom = 0;
	};

	void fetch();

	void toggleDeathMarkers();

	void updateStacks(float maxDistance);

	void analyzeData();

	void startUI();

	void updateMarkers(float);

};


class DMControlPanel : public CCLayer {

//private:
	//static DMControlPanel* instance;

protected:
	DMEditorLayer* m_editor = nullptr;
	CCSize m_size;
	cocos2d::extension::CCScale9Sprite* m_bgSprite = nullptr;
	bool setup(float anchorX, float anchorY, const char* bg);

public:
	static DMControlPanel* create();
	
};

#include <Geode/modify/EditorPauseLayer.hpp>
class $modify(DMEditorPauseLayer, EditorPauseLayer) {

	struct Fields {
		CCMenuItemSprite* m_button;
	};

	bool init(LevelEditorLayer * layer);

};