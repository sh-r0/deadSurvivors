#pragma once
#include <array>
#include <optional>

#include "guiTypes.hpp"
#include "gameTypes.hpp"
#include "utils.hpp"

class gameManager_t;

struct gameData_t {
	std::unordered_map<uint8_t, spell_t> spellMap; // potentially better to have it as arr or vec for continuity / !ensure that spellType has same type as spellMapKey		
	std::unordered_map<uint8_t, sprite_t> projectileData;
	std::unordered_map<uint16_t, enemyInfo_t> enemyData;
	std::unordered_map<uint8_t, pickupInfo_t> pickupData;

	playerStats_t upgrades; //smth like this 
    std::array<uint32_t, 6> upgradesCosts; //we make it so not every upgrade can be bought
    uint32_t gold;
};

struct levelData_t {
	objSize_t mapSize;
	playerInfo_t player;

	std::vector<enemy_t> enemies;		// info about enemy instance
	std::vector<obstacle_t> obstacles;			// ~15 per map
	std::vector<pickup_t> pickups;				//	~ 0 - 100+
	std::vector<projectile_t> enemyProjectiles;
	std::vector<projectile_t> playerProjectiles;
};

class levelManager_t {
public:
	gameManager_t* gameManager;	
	gameData_t* gameData;

	std::array<int8_t, 2> playerDir = { 0,0 }, lastPlayerDir = { 1,1 };
	position_t closestDir;	 //from player to enemy
	float closestDist;

	bool gamePaused = false;
	position_t cameraPos;
	double levelTime;
	int32_t enemyCount; //max of enemies on screen
	int32_t enemiesKilled;
	int32_t goldEarned;

    levelData_t level;

	void addSpell(spell_t& _spell);
	std::optional<spell_t*> getSpell(spellType _spellType);
	void initPlayer();
	void initLevel(); //load data into level
	void clearLevel(void);
	void spawnEnemies();
	void updateEnemies(double _dt);
	void updateGameState(double _dt);
};
