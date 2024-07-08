#include "gameManager.hpp"
#include <chrono>
#include <cstdlib>

//-------------------------------------------------------------------------------- class levelManager_t ----------------------------------------------------------------------------------------------------------------
#include <stdio.h>

void levelManager_t::initLevel(void) {
	playerInfo_t _player{};
	currLevel_.mapSize = { 4000, 4000 };
	_player.stats = {};
	_player.stats.speed = 256;
	_player.hitbox.position = { float(currLevel_.mapSize[0]/2), float(currLevel_.mapSize[1]/2) };
	_player.hitbox.size = { 64,64 };
	
	_player.spritePosition = _player.hitbox.position;
	_player.sprite.size = { 64,64 };
	_player.sprite.texId = 0;
	_player.sprite.texPos = { 0, 0 };
	_player.sprite.texSize = { 0.25, 0.25 };
	
	_player.maxHp = 100;
	_player.hp = 100;

	spell_t newSpell = {};
	newSpell.cooldownTime = 0.2; 
	newSpell.time = newSpell.cooldownTime; 
	newSpell.type = SPELL_TYPE_WHIP;
	newSpell.effect = [](levelManager_t& _level) {
		projectile_t whipStrike{};
		auto& playerHitbox = _level.currLevel_.player.hitbox;
		whipStrike.hitbox.position = { playerHitbox.position[0] + playerHitbox.size[0], playerHitbox.position[1] };
		whipStrike.hitbox.size = { 128, 64 };
		whipStrike.lifetime = 0.1;
		whipStrike.dmg = 40;

		whipStrike.spritePosition = whipStrike.hitbox.position;
		whipStrike.sprite.size = whipStrike.hitbox.size;
		whipStrike.sprite.texPos = { 0, 0.25 };
		whipStrike.sprite.texSize = { 0.5, 0.25 };
		if (!(_level.playerFacingRight_)) {
			whipStrike.sprite.texPos = { 0.5, 0.5 };
			whipStrike.sprite.texSize = { -0.5, -0.25 };
			whipStrike.move({ -1.0f * (whipStrike.hitbox.size[0] + playerHitbox.size[0]) , 0});
			//whipStrike.spritePosition[0] -= (whipStrike.hitbox.size[0] - playerHitbox.size[0]);
		}
		whipStrike.sprite.texId = 0;
		whipStrike.velocity = {};

		_level.currLevel_.playerProjectiles.push_back(whipStrike);
	};
	_player.spells.push_back(newSpell);
	currLevel_.player = _player;

	cameraPos_ = _player.spritePosition;

	{
		sprite_t redWalkerSprite{};
		redWalkerSprite.size = { 64,64 };
		redWalkerSprite.texId = 0;
		redWalkerSprite.texPos = { 0.25, 0.0 };
		redWalkerSprite.texSize = { 0.25, 0.25 };
		
		monsterStats_t stats{};
		stats.speed = 128;

		enemyInfo_t redWalkerInfo{};
		redWalkerInfo.hitboxSize = { 64,64 };
		redWalkerInfo.sprite = redWalkerSprite;
		redWalkerInfo.stats = stats;

		currLevel_.enemyData.insert({ENEMY_TYPE_RED_WALKER, redWalkerInfo});
	}
	{
		sprite_t goldSprite {};
		goldSprite.size = { 32,32 };
		goldSprite.texId = 0;
		goldSprite.texSize = {0.125,0.125};
		goldSprite.texPos = { 0, 1 - 0.125};

		pickupInfo_t goldPickup{};
		goldPickup.sprite = goldSprite;
		
		currLevel_.pickupData.insert({ PICKUP_TYPE_GOLD, goldPickup });

		sprite_t expSprite {};
		expSprite.size = { 32,32 };
		expSprite.texId = 0;
		expSprite.texSize = { 0.125,0.125 };
		expSprite.texPos = { 0.125, 1 - 0.125 };

		pickupInfo_t expPickup{};
		expPickup.sprite = expSprite;

		currLevel_.pickupData.insert({ PICKUP_TYPE_EXP, expPickup});
	}

	return;
}

inline bool checkForIntersection(const hitbox_t& _a, const hitbox_t& _b) {
	if (_b.position[0] + _b.size[0] > _a.position[0] && _b.position[0] < _a.position[0] + _a.size[0])
		if (_b.position[1] + _b.size[1] > _a.position[1] && _b.position[1] < _a.position[1] + _a.size[1]) 
			return true;
		else 
			return false;
	return false;
}

inline void updatePlayerPosition(levelManager_t& _level, double _dt) {
	auto& playerDir = _level.playerDir_;
	auto& playerFacingRight = _level.playerFacingRight_;
	auto& currLevel_= _level.currLevel_;
	auto& playerPos = currLevel_.player.hitbox.position;

	if (playerDir[0] != 0)
		playerFacingRight = playerDir[0] > 0;

	currLevel_.player.move({ float(currLevel_.player.stats.speed * playerDir[0] * _dt),
										  float(currLevel_.player.stats.speed * playerDir[1] * _dt), });

	if (playerPos[0] < 0) {
		playerPos[0] = 0;
		currLevel_.player.spritePosition = playerPos;
	}
	if (playerPos[1] < 0) {
		playerPos[1] = 0;
		currLevel_.player.spritePosition = playerPos;
	}
	if (playerPos[0] > currLevel_.mapSize[0]) {
		playerPos[0] = currLevel_.mapSize[0];
		currLevel_.player.spritePosition = playerPos;
	}
	if (playerPos[1] > currLevel_.mapSize[1]) {
		playerPos[1] = currLevel_.mapSize[1];
		currLevel_.player.spritePosition = playerPos;
	}

	return;
}

inline void updateSpells(levelManager_t& _level, double _dt) {
	for (auto& spell : _level.currLevel_.player.spells) {
		spell.time -= _dt;
		if (spell.time <= 0) {
			spell.time += spell.cooldownTime;
			spell.effect(_level);
		}
	}

	return;
}

inline void updateProjectiles(levelManager_t& _level, double _dt) {
	auto& currLevel_ = _level.currLevel_;
	for (size_t i = 0; i < currLevel_.playerProjectiles.size(); i++) {
		auto& proj = currLevel_.playerProjectiles[i];

		proj.lifetime -= _dt;
		if (proj.lifetime <= 0) {
			currLevel_.playerProjectiles.erase(currLevel_.playerProjectiles.begin() + i);
			i--;
			continue;
		}
		proj.move({ float(_dt * proj.velocity[0]),
						  float(_dt * proj.velocity[1]) });
	}

	return;
}

void updateEnemies(levelManager_t& _level, double _dt) {
	auto& currLevel_ = _level.currLevel_;
	auto& playerPos = currLevel_.player.hitbox.position;

	for (size_t i = 0; i < currLevel_.enemies.size(); i++) {
		auto& enemy = currLevel_.enemies[i];
		auto enemySpeed = currLevel_.enemyData[enemy.type].stats.speed;
		position_t moveDir = {};

		if (enemy.hitboxPosition[0] > playerPos[0])
			moveDir[0] = -1 * enemySpeed * _dt;
		else
			moveDir[0] = enemySpeed * _dt;
		if (enemy.hitboxPosition[1] > playerPos[1])
			moveDir[1] = -1 * enemySpeed * _dt;
		else
			moveDir[1] = enemySpeed * _dt;

		enemy.invincibilityTime -= _dt;
		if (enemy.invincibilityTime <= 0) {
			for (auto& proj : currLevel_.playerProjectiles) {
				bool isIntersecting = checkForIntersection(proj.hitbox, hitbox_t{.position = enemy.hitboxPosition, .size = currLevel_.enemyData[enemy.type].hitboxSize });
				if (isIntersecting) {
					enemy.hp -= proj.dmg;
					if (enemy.hp <= 0) {
						//todo: enemy.onDieEffect()
						pickup_t dropPickup = {};
						dropPickup.position = enemy.spritePosition;
						dropPickup.position[0] += currLevel_.enemyData[enemy.type].sprite.size[0] * 0.5;
						dropPickup.position[1] += currLevel_.enemyData[enemy.type].sprite.size[1] * 0.5;

						uint8_t roll = rand() % 10;
						if (roll > 8) PICKUP_TYPE_GOLD;
						else dropPickup.type = PICKUP_TYPE_EXP;
						currLevel_.pickups.push_back(dropPickup);

						currLevel_.enemies.erase(currLevel_.enemies.begin() + i);
						i--;
						goto flag_loop1end;
					}
					enemy.invincibilityTime = 0.25;
				}	// if
			}	// for
		}	// if
		enemy.move(moveDir);
	flag_loop1end:
		__noop;
	}
	
	while (currLevel_.enemies.size() < 50) {
		double phi = (rand() / double(RAND_MAX)) * 2 * 3.14159265;
		enemy_t enemy{};
		static constexpr uint16_t spawnRange = 1000;
		enemy.hitboxPosition = { float(playerPos[0] + spawnRange * sin(phi)), float(playerPos[1] + spawnRange * cos(phi)) };
		enemy.type = ENEMY_TYPE_RED_WALKER;
		enemy.hp = 100;

		enemy.spritePosition = enemy.hitboxPosition;
		currLevel_.enemies.push_back(enemy);
	}

	return;
}

inline bool checkForIntersection(const hitbox_t& _a, const position_t& _b) {
	if (_b[0] >= _a.position[0] && _b[0] <= (_a.position[0] + _a.size[0]))
		if (_b[1] >= _a.position[1] && _b[1] <= (_a.position[1] + _a.size[1]))
			return true;

	return false;
}

#include <iostream>
inline void updatePickups(levelManager_t& _level) {
	auto& player = _level.currLevel_.player;
	for (size_t i = 0; i < _level.currLevel_.pickups.size(); i++) {
		auto& pickup = _level.currLevel_.pickups[i];
		if (checkForIntersection(player.hitbox, pickup.position)) {
			switch (pickup.type) {
				case PICKUP_TYPE_EXP: {
					player.exp += 20;
				} break;
				case PICKUP_TYPE_GOLD: {
					player.gold += 20;
				} break;
			}

			_level.currLevel_.pickups.erase(_level.currLevel_.pickups.begin() + i);
			i--;
			continue;
		}
	}
	return;
}

void levelManager_t::updateGameState(double _dt) {
	//static std::chrono::steady_clock::time_point lastFrameEnd ;
	_dt /= 1000; // _dt = 1s
	
	auto& playerPos = currLevel_.player.hitbox.position;

	updatePlayerPosition(*this, _dt);
	updateSpells(*this, _dt);
	updateProjectiles(*this, _dt);
	updatePickups(*this);

	cameraPos_[0] += (playerPos[0] - cameraPos_[0]) * 0.9 * _dt;
	cameraPos_[1] += (playerPos[1] - cameraPos_[1]) * 0.9 * _dt;

	updateEnemies(*this, _dt);

	return;
}

//-------------------------------------------------------------------------------- class gameManager_t ------------------------------------------------------------------------------------------------------------
void initGame(void) {

	return;
}