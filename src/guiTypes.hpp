#pragma once

#include <string>
#include <vector>
#include <vector>
#include "gameTypes.hpp"

class gameManager_t;

enum guiType : uint8_t {
	GUI_TYPE_SPRITE,
	GUI_TYPE_BUTTON,
	GUI_TYPE_BAR,
	GUI_TYPE_TEXT,
};

struct guiButton_t {
	position_t spritePosition;
	sprite_t sprite;
	hitbox_t area;

	uint32_t id;
	void(*onClick)(gameManager_t&, uint32_t _id);
};

enum textPositionType : uint8_t {
	TEXT_POSITION_TYPE_LEFT,
	TEXT_POSITION_TYPE_CENTRE,
	TEXT_POSITION_TYPE_RIGHT,
};

struct guiText_t {
	std::string text;
	position_t position;
	uint8_t positionType;
};

struct guiBar_t {
	position_t position;
	float filledPercent;
	sprite_t background;
	sprite_t foreground;
	bool horizontal;
};

struct guiSprite_t {
	position_t position;
	sprite_t sprite;
};

struct guiElement_t {
	uint8_t type;
	void* data;	
};