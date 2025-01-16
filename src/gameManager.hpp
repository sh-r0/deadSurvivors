#pragma once

#include "gameTypes.hpp"
#include "guiTypes.hpp"
#include "levelManager.hpp"

enum layoutType : uint8_t {
	LAYOUT_TYPE_GAME_SCREEN,
	LAYOUT_TYPE_PAUSE_GAME,
	LAYOUT_TYPE_SPELL_CHOICE,
	LAYOUT_TYPE_GAME_OVER, 
	LAYOUT_TYPE_MAIN_MENU
};

struct guiLayout_t {
	uint8_t layoutType; // layoutType
	gameManager_t* managerPtr;
	std::vector<guiElement_t> guiElements;
	std::vector<uint8_t> options;

	void (*processInput)(gameManager_t&);	// can be nullptr
	void (*initLayout)(gameManager_t&);		// can be nullptr
	void (*updateLayout)(gameManager_t&);	//must be specified!
};

class gameManager_t {
public:
	uint32_t winSizeX, winSizeY;
	gameData_t gameData_;
	levelManager_t levelManager_;

	std::vector<guiLayout_t> layouts_;
	uint8_t currLayout_;
	uint8_t unpausedLayout_;
	bool closeGame_ = false;

	inline guiLayout_t& getCurrLayout() {
		return layouts_[currLayout_];
	}

	void initGame(); // maybe todo: loading data from json
	void updateGameState(double);
	void processMouseInput(position_t);

private: 
	void loadLayouts(void);
	void loadEnemyData(void);
	void loadProjectileData(void);
	void loadPickupData(void);
	void loadGameData(void);
	void loadSpells(void);
};