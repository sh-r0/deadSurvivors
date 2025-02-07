#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <unordered_map>

typedef std::array<float, 2> position_t;
typedef std::array<uint32_t, 2> objSize_t;
typedef std::array<float, 2> float2_t;

struct hitbox_t {
	position_t position;
	objSize_t size;
};

struct sprite_t {
	objSize_t size;

	uint16_t texId;
	float2_t texPos;
	float2_t texSize;
};

struct playerStats_t {
	int16_t speed;
	int16_t attackSpeed;
	int16_t defense;
	int16_t attack;
	int16_t magic;
	int16_t luck;
};

struct monsterStats_t {
	int16_t speed;
	int16_t attackSpeed;
	int16_t defense;
	int16_t attack;
	int16_t maxHp;
};

struct levelData_t;
class levelManager_t;

enum spellType : uint8_t {
	SPELL_TYPE_WHIP,
	SPELL_TYPE_BOW,
	SPELL_TYPE_SWORD,
	SPELL_TYPE_FIRE_ORB,

	SPELL_TYPE_MAX
};

struct spell_t;
using effect_t = void(*)(levelManager_t&, spell_t&);

struct spell_t {
	spellType type;
	uint8_t level;
	double cooldownTime; // seconds
	double time; // current time in seconds
	effect_t effect;
};

struct playerInfo_t {
	playerStats_t stats;
	uint64_t exp;
	uint64_t expTillNextLvl = 100;
	uint8_t lvl = 0;
	uint32_t gold;
	hitbox_t hitbox;
	sprite_t sprite;
	position_t spritePosition;
	int32_t hp;
	int32_t maxHp;
	double invincibilityTime = 0;
	double invincibilityTimeShield = .2;
	bool isTurnedRight = false;

	std::vector<spell_t> spells;
	std::unordered_map<spellType, uint8_t> spellIndexes;
	//uint8_t spellLevels[SPELL_TYPE_MAX];

	std::vector<void(*)(void)> onHitEffects;

	void move(position_t _distance) {
		hitbox.position[0] += _distance[0];
		hitbox.position[1] += _distance[1];
		spritePosition[0] += _distance[0];
		spritePosition[1] += _distance[1];

		return;
	}
};

struct enemyInfo_t {
	monsterStats_t stats;
	objSize_t hitboxSize;
	//objSize_t hitboxOffset;
	sprite_t sprite;
};

struct pickupInfo_t {
	sprite_t sprite;
};

enum enemyType : uint8_t {
	ENEMY_TYPE_PLAYER,
	ENEMY_TYPE_SLIME_BLUE,
	ENEMY_TYPE_SLIME_GREEN,
	ENEMY_TYPE_SKELETON,
	ENEMY_TYPE_GHOST,

	ENEMY_TYPE_MAX,
};

enum projectileType : uint8_t {
	PROJECTILE_TYPE_WHIP,
	PROJECTILE_TYPE_FLAME,
	PROJECTILE_TYPE_ARROW,
	PROJECTILE_TYPE_SWORD_X,
	PROJECTILE_TYPE_SWORD_Y,

	PROJECTILE_TYPE_MAP,
};

struct enemy_t {
	uint16_t type;
	position_t hitboxPosition;
	position_t spritePosition;
	int32_t hp;
	bool isTurnedRight;
	double invincibilityTime;

	void move(position_t _distance) {
		hitboxPosition[0] += _distance[0];
		hitboxPosition[1] += _distance[1];
		spritePosition[0] += _distance[0];
		spritePosition[1] += _distance[1];

		return;
	}
};


struct projectile_t {
	hitbox_t hitbox;
	sprite_t sprite;
	position_t spritePosition;
	uint32_t dmg;

	position_t velocity;
	double lifetime; // how much longer to live in seconds
	bool isHitDependent;
	int8_t hits;

	void move(position_t _distance) {
		hitbox.position[0] += _distance[0];
		hitbox.position[1] += _distance[1];
		spritePosition[0] += _distance[0];
		spritePosition[1] += _distance[1];

		return;
	}
};

struct obstacle_t {
	hitbox_t hitbox;
	sprite_t sprite;
};

enum pickupType : uint8_t {
	PICKUP_TYPE_GOLD,
	PICKUP_TYPE_GREATER_GOLD,
	PICKUP_TYPE_EXP,
	PICKUP_TYPE_GREATER_EXP,
	PICKUP_TYPE_CHEST,
	PICKUP_TYPE_GREATER_CHEST
};

struct pickup_t {
	uint8_t type;		//pickupType
	position_t position;	//center of sprite
};
