#pragma once

#include <vector>
#include <array>
//#include <functional>
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

	uint8_t frame;
	uint8_t maxFrames;
};

struct playerStats_t {
	int32_t speed;
	int32_t attackSpeed;
	int32_t defense;
	int32_t attack;
	int32_t magic;
	int32_t luck;
};

struct monsterStats_t {
	int32_t speed;
	int32_t attackSpeed;
	int32_t defense;
	int32_t attack;
};

struct levelData_t;
class levelManger_t;

enum spellType : uint8_t {
	SPELL_TYPE_WHIP,
};

struct spell_t {
	uint8_t type;
	double cooldownTime; // milliseconds
	double time; //
	void(*effect)(class levelManager_t&);
};

struct playerInfo_t {
	playerStats_t stats;
	uint32_t exp;
	uint32_t gold;
	hitbox_t hitbox;
	sprite_t sprite;
	position_t spritePosition;
	uint32_t hp;
	uint32_t maxHp;
	double invincibilityTime = 0;

	std::vector<spell_t> spells;
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
	sprite_t sprite;	
};

struct pickupInfo_t {
	sprite_t sprite;
};

enum enemyType : uint16_t {
	ENEMY_TYPE_RED_WALKER,
};

struct enemy_t {
	uint16_t type;				
	position_t hitboxPosition;	
	position_t spritePosition;
	int32_t hp;
	double invincibilityTime = 0;

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
	position_t position;	// center of sprite

};

struct levelData_t {
	objSize_t mapSize;
	std::unordered_map<uint16_t, enemyInfo_t> enemyData;	
	std::unordered_map<uint8_t, pickupInfo_t> pickupData;

	std::vector<enemy_t> enemies;		// info about enemy instance
	playerInfo_t player;

	std::vector<obstacle_t> obstacles;			// ~15 per map
	std::vector<pickup_t> pickups;				//	~ 0 - 100+
	std::vector<projectile_t> enemyProjectiles;
	std::vector<projectile_t> playerProjectiles;
};

struct gameData_t {
	std::vector<uint32_t> upgrades; //smth like this 
};

struct scene_t {
	//ui 
};

class levelManager_t {
public: 
	std::array<int8_t, 2> playerDir_ = {0,0};
	bool playerFacingRight_ = true;
	position_t cameraPos_;

	levelData_t currLevel_;

	void initLevel(); // load data into level
	void updateGameState(double);
};

class gameManager_t {
public:
	gameData_t gameData_;
	scene_t currScene_;

	void initGame(); // todo: loading from save file
	


};