#pragma once
#include <optional>

#include "guiTypes.hpp"
#include "gameTypes.hpp"
#include "utils.hpp"

class gameManager_t;

struct gameData_t {
	std::unordered_map<uint8_t, spell_t> spellMap; // potentially better to have it as arr or vec for continuity / !ensure that spellType has same type as spellMapKey		
	std::unordered_map<uint16_t, enemyInfo_t> enemyData;
	std::unordered_map<uint8_t, pickupInfo_t> pickupData;
	
	std::vector<uint32_t> upgrades; //smth like this 
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
	gameManager_t* gameManager_;	
	gameData_t* gameData_;

	std::array<int8_t, 2> playerDir_ = { 0,0 }, lastPlayerDir_ = { 1,1 };
	position_t closestDir_;	 //from player to enemy
	float closestDist_;

	bool gamePaused_ = false;
	position_t cameraPos_;
	double levelTime_;
	int32_t enemyCount_; // max of enemies on screen
	int32_t enemiesKilled_;
	levelData_t level_;

	void addSpell(spell_t& _spell);
	std::optional<spell_t*> getSpell(spellType _spellType);
	void initLevel(); // load data into level
	void clearLevel(void);
	void updateGameState(double);
};