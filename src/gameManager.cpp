#include "gameManager.hpp"
#include "utils.hpp"
#include "spells.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <type_traits>
#include <unordered_set>
#include <format>

#define SAME_TYPE(x, y) (std::is_same<x,y>().value)

template<typename T>
constexpr inline guiElement_t wrapGui(T* _elePtr) {
	guiElement_t res {};

	if constexpr (SAME_TYPE(T, guiBar_t))
		res.type = GUI_TYPE_BAR;
	else if constexpr (SAME_TYPE(T, guiButton_t))
		res.type = GUI_TYPE_BUTTON;
	else if constexpr (SAME_TYPE(T, guiSprite_t))
		res.type = GUI_TYPE_SPRITE;
	else if constexpr (SAME_TYPE(T, guiText_t))
		res.type = GUI_TYPE_TEXT;
	else res.type = 0;
	
	res.data = _elePtr;
	return res;
}

inline bool checkForIntersection(const hitbox_t& _a, const hitbox_t& _b) {
	if (_b.position[0] + _b.size[0] > _a.position[0] && _b.position[0] < _a.position[0] + _a.size[0])
		if (_b.position[1] + _b.size[1] > _a.position[1] && _b.position[1] < _a.position[1] + _a.size[1])
			return true;
		else
			return false;
	return false;
}

inline bool checkForIntersection(const hitbox_t& _a, const position_t& _b) {
	if (_b[0] >= _a.position[0] && _b[0] <= (_a.position[0] + _a.size[0]))
		if (_b[1] >= _a.position[1] && _b[1] <= (_a.position[1] + _a.size[1]))
			return true;

	return false;
}

void loadGameScreenLayout(gameManager_t& _gm) {
	guiLayout_t layout{};
	layout.layoutType = LAYOUT_TYPE_GAME_SCREEN;
	layout.managerPtr = &_gm;

	guiBar_t* bar = new guiBar_t;
	*bar = {};
	bar->background.size = { 208, 16 };
	bar->background.texId = 1;
	bar->background.texPos = { 96 / 512.0f, 0.0f };
	bar->background.texSize = { 208 / 512.0f, 16 / 512.0f };
	bar->foreground.size = { 208, 16 };
	bar->foreground.texId = 1;
	bar->foreground.texPos = { 96 / 512.0f, 16.0f / 512.0f };
	bar->foreground.texSize = { 208 / 512.0f, 16 / 512.0f };
	bar->position = { 96, 300 - 16 };
	bar->horizontal = true;

	guiText_t* expText = new guiText_t;
	expText->position = { 96 , 300 - 16 - 2 };
	expText->positionType = TEXT_POSITION_TYPE_LEFT;
	expText->text = "";

	guiSprite_t* hpIcon = new guiSprite_t;
	*hpIcon = {
		.position = {288,300 - 16 - 16},
		.sprite = {
			.size = {16,16},
			.texId = 1,
			.texPos = {8 / 512.0f, 208 / 512.0f},
			.texSize = {8 / 512.0f, 8 / 512.0f}
		}
	};

	guiText_t* hpText = new guiText_t;
	hpText->position = { 288,300-16-2 };
	hpText->positionType = TEXT_POSITION_TYPE_RIGHT;
	hpText->text = "";//health level

	guiSprite_t* killsIcon = new guiSprite_t;
	*killsIcon = {
		.position = {400-16, 16},
		.sprite = {
			.size = {16,16},
			.texId = 1,
			.texPos = {0 / 512.0f, 208 / 512.0f},
			.texSize = {8 / 512.0f, 8 / 512.0f}
		}
	};

	guiText_t* killsText = new guiText_t;
	*killsText = {
		.text = "",		
		.position = { 400-16, 32 - 3},
		.positionType = TEXT_POSITION_TYPE_RIGHT,
	};
	
	guiText_t* timerText = new guiText_t;
	*timerText = {
		.text = "0:00",
		.position = {400 - 200, 18},
		.positionType = TEXT_POSITION_TYPE_CENTRE
	};

	layout.guiElements.push_back(wrapGui(bar));
	layout.guiElements.push_back(wrapGui(expText));
	layout.guiElements.push_back(wrapGui(hpIcon));
	layout.guiElements.push_back(wrapGui(hpText));
	layout.guiElements.push_back(wrapGui(killsIcon));
	layout.guiElements.push_back(wrapGui(killsText));
	layout.guiElements.push_back(wrapGui(timerText));

	layout.updateLayout = [](gameManager_t& _mng) {
		auto& layout = _mng.layouts_[_mng.currLayout_];
		auto& xpText = *(guiText_t*)layout.guiElements[1].data;
		auto& hpText = *(guiText_t*)layout.guiElements[3].data;
		auto& killsText = *(guiText_t*)layout.guiElements[5].data;
		auto& timerText = *(guiText_t*)layout.guiElements[6].data;

		xpText.text = std::format("{}/{}", _mng.levelManager_.level_.player.exp, _mng.levelManager_.level_.player.expTillNextLvl);
		hpText.text = std::format("{}/{}", _mng.levelManager_.level_.player.hp, _mng.levelManager_.level_.player.maxHp);
		killsText.text = std::format("{}", _mng.levelManager_.enemiesKilled_);

		int lvlSeconds = (int)_mng.levelManager_.levelTime_;
		timerText.text = std::format("{}:{}{}", lvlSeconds/60, lvlSeconds%60/10, lvlSeconds%10);

		auto& xpBar = *(guiBar_t*)layout.guiElements[0].data;
		xpBar.filledPercent = float(_mng.levelManager_.level_.player.exp) / _mng.levelManager_.level_.player.expTillNextLvl;
	};

	layout.processInput = [](gameManager_t&) {};

	_gm.layouts_.push_back(layout);

	return;
}

void loadPauseGameLayout(gameManager_t& _gm) {
	guiLayout_t layout = {};
	layout.layoutType = LAYOUT_TYPE_PAUSE_GAME;
	layout.managerPtr = &_gm;
	layout.options = {};
	layout.guiElements = {};
	layout.initLayout = [](gameManager_t& _gm) {};
	layout.processInput = [](gameManager_t& _gm) {};
	layout.updateLayout = [](gameManager_t& _gm) {};

	for (auto& guiEle : _gm.layouts_[0].guiElements) 
		layout.guiElements.push_back(guiEle);
	
	const static std::array<std::string, 2> texts = { "Continue", "Exit" };
	static void (*functions[])(gameManager_t&, uint32_t) = {
		[](gameManager_t& _gm, uint32_t _id) { _gm.currLayout_ = _gm.unpausedLayout_; },
		[](gameManager_t& _gm, uint32_t _id) { _gm.currLayout_ = LAYOUT_TYPE_MAIN_MENU; _gm.levelManager_.clearLevel(); },
	};

	for (size_t i = 0; i < texts.size(); i++) {
		guiButton_t* button = new guiButton_t;
		button->sprite = {};
		button->sprite.size = { 100, 50 };
		button->sprite.texId = 1;
		button->sprite.texPos = { 96 / 512.0f, 32 / 512.0f };
		button->sprite.texSize = { 100 / 512.0f, 50 / 512.0f };
		button->spritePosition = { 150.0f , 100.0f + 75 * i };
		button->area.position = button->spritePosition;
		button->area.size = button->sprite.size;
		button->onClick = functions[i];
		layout.guiElements.push_back(wrapGui(button));

		guiText_t* gText = new guiText_t;
		gText->position = { 200.0f , 130.0f + 75 * i };
		gText->positionType = TEXT_POSITION_TYPE_CENTRE;
		gText->text = texts[i];
		layout.guiElements.push_back(wrapGui(gText));
	}

	_gm.layouts_.push_back(layout);

	return;
}

void loadSpellChoiceLayout(gameManager_t& _gm) {
	guiLayout_t layout = {};
	layout.layoutType = LAYOUT_TYPE_SPELL_CHOICE;
	layout.managerPtr = &_gm;
	layout.options = {};
	layout.initLayout = [](gameManager_t& _gm) {
		auto& thisLayout = _gm.layouts_[_gm.currLayout_];
		thisLayout.options.clear();
		for (size_t i = 0; i < 3; i++) {
			std::unordered_set<uint8_t> otherOptions;
			for (auto _other : thisLayout.options) otherOptions.insert(_other);
			uint8_t opt = rand() % SPELL_TYPE_MAX;
			while (otherOptions.find(opt) != otherOptions.end()) opt = (opt + 1) % SPELL_TYPE_MAX;

			thisLayout.options.push_back(opt);

			auto& iconSprite = *(guiSprite_t*)thisLayout.guiElements[i * 3 + 1].data;
			iconSprite.sprite.texPos[0] = 32 / 512.0f * opt;

			auto& lvlText = *(guiText_t*)thisLayout.guiElements[i * 3 + 2].data;
			std::optional<spell_t*> _spell = _gm.levelManager_.getSpell((spellType)opt);
			lvlText.text = std::format("{} lvl", _spell.has_value() ? _spell.value()->level : 0);
		}
	};

	layout.updateLayout = [](gameManager_t& _gm) {};

	//potentially should change guiElements to unordered map for the sake of sanity but for now:
	//DONT CHANGE ORDER OR CHANGE INIT LAYOUT ABOVE ^^^
	for (size_t i = 0; i < 3; i++) {
		guiButton_t* button = new guiButton_t;
		guiSprite_t* sprite = new guiSprite_t;		
		guiText_t* text = new guiText_t;

		//whole area with text and icon on top
		button->spritePosition = { 28.0f + 124 * i, 32 };
		button->sprite = {};
		button->sprite.size = { 96, 144 };
		button->sprite.texId = 1;
		button->sprite.texPos = { 0.0, 0.0 };
		button->sprite.texSize = { 96 / 512.0f, 144.0f / 512.0f };
		button->id = i;

		button->area.position = { button->spritePosition[0] + 6, button->spritePosition[1] + 6 };
		button->area.size = { button->sprite.size[0] - 12, button->sprite.size[1] - 12 };

		button->onClick = [](gameManager_t& _gm, uint32_t _id) {
			auto& player = _gm.levelManager_.level_.player;
			const auto option = _gm.getCurrLayout().options[_id];

			std::optional<spell_t*> spell = _gm.levelManager_.getSpell((spellType)option);
			if (!spell.has_value()) 
				_gm.levelManager_.addSpell(_gm.gameData_.spellMap[(spellType)option]);

			upgradeSpell(*_gm.levelManager_.getSpell(spellType(option)).value());
			//_gm.levelManager_.getSpell((spellType)option).value()->level += 1;	//prevents lvl 0

			_gm.currLayout_ = LAYOUT_TYPE_GAME_SCREEN;
			};

		// icon of option
		sprite->position = { button->spritePosition[0] + 32, button->spritePosition[1] + 24 };
		sprite->sprite = {};
		sprite->sprite.size = { 32, 32 };
		sprite->sprite.texId = 1;
		sprite->sprite.texPos = { 0.0f / 512.0f, 176.0f / 512.0f };
		sprite->sprite.texSize = { 32.0f / 512.0f, 32.0f / 512.0f };

		text->positionType = TEXT_POSITION_TYPE_CENTRE;
		text->position = sprite->position;
		text->position[1] += 20 + sprite->sprite.size[1];
		text->position[0] += sprite->sprite.size[0] / 2;

		layout.guiElements.push_back(wrapGui(button));
		layout.guiElements.push_back(wrapGui(sprite));
		layout.guiElements.push_back(wrapGui(text));
	}

	_gm.layouts_.push_back(layout);

	return;
}

void loadGameOverLayout(gameManager_t& _gm) {
	guiLayout_t layout{};
	layout.managerPtr = &_gm;
	layout.layoutType = LAYOUT_TYPE_GAME_OVER;
	layout.updateLayout = [](gameManager_t& _gm) {
		_gm.levelManager_.clearLevel();
		return;
	};

	guiText_t* text = new guiText_t();
	*text = {
		.text = "Game over!",
		.position = {200,100},
		.positionType = TEXT_POSITION_TYPE_CENTRE
	};

	guiButton_t* button = new guiButton_t(); 
	*button = {
		.spritePosition = position_t { 150, 175 },
		.sprite = sprite_t {
			.size = { 100,50 },
			.texId = 1,
			.texPos = { 96/512.0f, 32/512.0f },
			.texSize = { 100/512.0f, 50/512.0f }
		},
		.area = {
			position_t { 150, 200 },
			objSize_t { 100, 50 }
		},
		.id = 0,
		.onClick = [](gameManager_t& _gm, uint32_t id) {
			_gm.currLayout_ = LAYOUT_TYPE_MAIN_MENU;
			return;
		}
	};

	guiText_t* exitText = new guiText_t;
	*exitText = {
		.text = "Exit",
		.position = {200, 205},
		.positionType = TEXT_POSITION_TYPE_CENTRE
	};

	layout.guiElements.push_back(wrapGui(text));
	layout.guiElements.push_back(wrapGui(button));
	layout.guiElements.push_back(wrapGui(exitText));

	_gm.layouts_.push_back(layout);

	return;
}

void loadMainMenuLayout(gameManager_t& _gm) {
	guiLayout_t layout{};
	layout.layoutType = LAYOUT_TYPE_MAIN_MENU;
	layout.managerPtr = &_gm;
	layout.updateLayout = [](gameManager_t& _gm) {};

	guiButton_t* startButton = new guiButton_t;
	guiButton_t* exitButton = new guiButton_t;
	guiText_t* startText = new guiText_t;
	guiText_t* exitText = new guiText_t;

	//position_t startButtonPosition = { 150, 100 };
	//objSize_t startButtonSize = { 100, 50 };

	*startText = guiText_t {
		.text = "Start",
		.position = { 200, 130 },
		.positionType = TEXT_POSITION_TYPE_CENTRE,
	}; 

	*exitText = guiText_t{
		.text = "Exit",
		.position = {200, 205},
		.positionType = TEXT_POSITION_TYPE_CENTRE,
	};

	auto buttonSprite = sprite_t{
			.size = { 100,50 },
			.texId = 1,
			.texPos = {96 / 512.0f, 32 / 512.0f},
			.texSize = {100 / 512.0f, 50 / 512.0f}
	};

	*startButton = {
		.spritePosition = {150, 100},
		.sprite = buttonSprite,
		.area = {
			position_t {150, 100},
			objSize_t {100, 50}
		},
		.id = 0,
		.onClick = [](gameManager_t& _gm, uint32_t _id) {
			_gm.levelManager_.initLevel();
			_gm.unpausedLayout_ = LAYOUT_TYPE_GAME_SCREEN;
			_gm.currLayout_ = LAYOUT_TYPE_SPELL_CHOICE;
			_gm.layouts_[_gm.currLayout_].initLayout(_gm);
			return;
		}
	};

	*exitButton = {
		.spritePosition = {150, 175},
		.sprite = buttonSprite,
		.area = {
			position_t {150, 175},
			objSize_t {100, 50}
		},
		.id = 0,
		.onClick = [](gameManager_t& _gm, uint32_t _id) {
			_gm.closeGame_ = true;
			return;
		}
	};

	layout.guiElements.push_back(wrapGui(startButton));
	layout.guiElements.push_back(wrapGui(startText));

	layout.guiElements.push_back(wrapGui(exitButton));
	layout.guiElements.push_back(wrapGui(exitText));

	_gm.layouts_.push_back(layout);

	return;
}

void gameManager_t::loadLayouts(void) {
	//order matters here! it shouldnt!
	loadGameScreenLayout(*this);
	loadPauseGameLayout(*this);
	loadSpellChoiceLayout(*this);
	loadGameOverLayout(*this);
	loadMainMenuLayout(*this);

	return;
}

void gameManager_t::loadSpells() {
	spell_t whip {};
	whip.type = SPELL_TYPE_WHIP;
	whip.cooldownTime = 3;
	whip.time = 0;
	whip.effect = _whipEffect;
	gameData_.spellMap.insert({SPELL_TYPE_WHIP ,whip});
	
	spell_t bow{};
	bow.type = SPELL_TYPE_BOW;
	bow.cooldownTime = 1.5f;
	bow.time = 0;
	bow.effect = _bowEffect;
	gameData_.spellMap.insert({ bow.type ,bow });
	
	spell_t fireOrb{};
	fireOrb.type = SPELL_TYPE_FIRE_ORB;
	fireOrb.cooldownTime = 1.5f;
	fireOrb.time = 0;
	fireOrb.effect = _fireOrbEffect;
	gameData_.spellMap.insert({ fireOrb.type ,fireOrb });
	
	spell_t ironSword {};
	ironSword.type = SPELL_TYPE_SWORD;
	ironSword.cooldownTime = 1.5f;
	ironSword.effect = _swordEffect;
	gameData_.spellMap.insert({ ironSword.type ,ironSword});

	return;
}

void gameManager_t::loadEnemyData(void) {
	{
		sprite_t sprite{};
		sprite.size = { 9,13 };
		sprite.texId = 0;
		sprite.texPos = { 0 / 256.0f, 0 / 256.0f };
		sprite.texSize = { 9 / 256.0f, 13 / 256.0f };

		enemyInfo_t playerInfo{};
		playerInfo.hitboxSize = { 9,13 };
		playerInfo.sprite = sprite;
		playerInfo.stats = {};

		gameData_.enemyData.insert({ ENEMY_TYPE_PLAYER, playerInfo });
	}
	{
		sprite_t sprite{};
		sprite.size = { 14, 9 };
		sprite.texId = 0;
		sprite.texPos = { 0 / 256.0f, 16 / 256.0f };
		sprite.texSize = { 14 / 256.0f, 9 / 256.0f };

		monsterStats_t stats{};
		stats.attack = 5;
		stats.speed = 32;
		stats.defense = 0;
		stats.maxHp = 50;

		enemyInfo_t blueSlimeInfo{};
		blueSlimeInfo.hitboxSize = { 14,9 };
		blueSlimeInfo.sprite = sprite;
		blueSlimeInfo.stats = stats;

		gameData_.enemyData.insert({ ENEMY_TYPE_SLIME_BLUE, blueSlimeInfo });
	}
	{
		sprite_t sprite{};
		sprite.size = { 16,11 };
		sprite.texId = 0;
		sprite.texPos = { 0 / 256.0f, 32 / 256.0f };
		sprite.texSize = { 16 / 256.0f, 11 / 256.0f };

		monsterStats_t stats{};
		stats.attack = 10;
		stats.speed = 32;
		stats.defense = 5;
		stats.maxHp = 100;

		enemyInfo_t greenSlimeInfo{};
		greenSlimeInfo.hitboxSize = { 16,11 };
		greenSlimeInfo.sprite = sprite;
		greenSlimeInfo.stats = stats;

		gameData_.enemyData.insert({ ENEMY_TYPE_SLIME_GREEN, greenSlimeInfo });
	}
	{
		sprite_t sprite{};
		sprite.size = { 14,13 };
		sprite.texId = 0;
		sprite.texPos = { 0 / 256.0f, 48 / 256.0f };
		sprite.texSize = { 14 / 256.0f, 13 / 256.0f };

		monsterStats_t stats{};
		stats.attack = 15;
		stats.speed = 32;
		stats.defense = 15;
		stats.maxHp = 75;

		enemyInfo_t skeletonInfo{};
		skeletonInfo.hitboxSize = { 14,13 };
		skeletonInfo.sprite = sprite;
		skeletonInfo.stats = stats;

		gameData_.enemyData.insert({ ENEMY_TYPE_SKELETON, skeletonInfo });
	}
	{
		sprite_t sprite{};
		sprite.size = { 10,13 };
		sprite.texId = 0;
		sprite.texPos = { 0 / 256.0f, 64 / 256.0f };
		sprite.texSize = { 10 / 256.0f, 13 / 256.0f };

		monsterStats_t stats{};
		stats.attack = 20;
		stats.speed = 32;
		stats.defense = 10;
		stats.maxHp = 1000;

		enemyInfo_t ghostInfo{};
		ghostInfo.hitboxSize = { 10,13 };
		ghostInfo.sprite = sprite;
		ghostInfo.stats = stats;

		gameData_.enemyData.insert({ ENEMY_TYPE_GHOST, ghostInfo });
	}

	return;
}

void gameManager_t::loadProjectileData(void) {
	{
		sprite_t whip{};
		whip.texId = 0;
		whip.texPos = { 128 / 256.0f, 0 / 256.0f };
		whip.texSize = { 29 / 256.0f, 9 / 256.0f };
		whip.size = { 29, 9 };
	
		gameData_.projectileData.insert({ PROJECTILE_TYPE_WHIP, whip });
	}
	{
		sprite_t swordSweep{};
		swordSweep.texId = 0;
		swordSweep.texPos = { 128 / 256.0f, 16 / 256.0f };
		swordSweep.texSize = { 27 / 256.0f, 11 / 256.0f };
		swordSweep.size = { 27, 11 };

		gameData_.projectileData.insert({ PROJECTILE_TYPE_SWORD_X, swordSweep });
	}
	{
		sprite_t swordSweep{};
		swordSweep.texId = 0;
		swordSweep.texPos = { 128 / 256.0f, 32 / 256.0f };
		swordSweep.texSize = { 11 / 256.0f, 27 / 256.0f };
		swordSweep.size = { 11, 27 };

		gameData_.projectileData.insert({ PROJECTILE_TYPE_SWORD_Y, swordSweep });
	}
	{
		sprite_t flame{};
		flame.texId = 0;
		flame.texPos = { 128 / 256.0f, 64/ 256.0f };
		flame.texSize = { 8 / 256.0f, 8 / 256.0f };
		flame.size = { 8,8 };

		gameData_.projectileData.insert({ PROJECTILE_TYPE_FLAME, flame });
	}
	{
		sprite_t arrow{};
		arrow.texId = 0;
		arrow.texPos = { 128 / 256.0f, 72 / 256.0f };
		arrow.texSize = { 10 / 256.0f, 3 / 256.0f };
		arrow.size = { 10, 3 };

		gameData_.projectileData.insert({ PROJECTILE_TYPE_ARROW, arrow });
	}

	return;
}

void gameManager_t::loadPickupData(void) {
	sprite_t goldSprite{};
	goldSprite.size = { 8,8 };
	goldSprite.texId = 0;
	goldSprite.texPos = { 128 / 256.0f, 88 / 256.0f };
	goldSprite.texSize = { 8 / 256.0f,8 / 256.0f };

	pickupInfo_t goldPickup{};
	goldPickup.sprite = goldSprite;

	gameData_.pickupData.insert({ PICKUP_TYPE_GOLD, goldPickup });

	sprite_t expSprite{};
	expSprite.size = { 8, 8 };
	expSprite.texId = 0;
	expSprite.texPos = { 128 / 256.0f, 96 / 256.0f };
	expSprite.texSize = { 8 / 256.0f, 8 / 256.0f };

	pickupInfo_t expPickup{};
	expPickup.sprite = expSprite;

	gameData_.pickupData.insert({ PICKUP_TYPE_EXP, expPickup });

	sprite_t gExpSprite{};
	gExpSprite.size = { 8, 8 };
	gExpSprite.texId = 0;
	gExpSprite.texPos = { 128 / 256.0f, 104 / 256.0f };
	gExpSprite.texSize = { 8 / 256.0f, 8 / 256.0f };

	pickupInfo_t gExpPickup{};
	gExpPickup.sprite = gExpSprite;

	gameData_.pickupData.insert({ PICKUP_TYPE_GREATER_EXP, gExpPickup });

	return;
}

void gameManager_t::loadGameData(void) {
	loadEnemyData();
	loadProjectileData();
	loadPickupData();

	return;
}

void gameManager_t::initGame(void) {
	loadLayouts();
	loadSpells();
	loadGameData();

	currLayout_ = LAYOUT_TYPE_MAIN_MENU;
	levelManager_.gameManager_ = this;
	levelManager_.gameData_ = &gameData_;

	//levelManager_.initLevel();
	return;
}

void gameManager_t::updateGameState(double _dt) {
	if(currLayout_ == LAYOUT_TYPE_GAME_SCREEN)
		levelManager_.updateGameState(_dt);
	layouts_[currLayout_].updateLayout(*this);

	return;
}

void gameManager_t::processMouseInput(position_t _mousePos) {
	if (currLayout_ == LAYOUT_TYPE_GAME_SCREEN)
		return;

	_mousePos = { (_mousePos[0] / winSizeX) * 400, (_mousePos[1] / winSizeY * 300) };

	for (guiElement_t& ele : layouts_[currLayout_].guiElements) {
		if (ele.type != GUI_TYPE_BUTTON) continue;
		
		auto& button = *(guiButton_t*)ele.data;
		if (checkForIntersection(button.area, _mousePos)) {
			button.onClick(*this, button.id);
		}
	}

	return;
}
