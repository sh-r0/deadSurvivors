#include "gameManager.hpp"
#include "levelManager.hpp"
#include "gameTypes.hpp"
#include "spells.hpp"

#include <cmath>
#include <format>
#include <iostream>
#include <stdio.h>
#include <optional>
#include <algorithm>

void levelManager_t::addSpell(spell_t& _spell) {
	level.player.spellIndexes.insert({ _spell.type, level.player.spells.size() });
	level.player.spells.push_back(_spell);

	return;
}

std::optional<spell_t*> levelManager_t::getSpell(spellType _spellType) {
	if (level.player.spellIndexes.find(_spellType) == level.player.spellIndexes.end())
		return std::nullopt;

	uint8_t index = level.player.spellIndexes.at(_spellType);
	return &level.player.spells[index];
}

void levelManager_t::initPlayer() {
	playerInfo_t _player{};
	_player.stats = gameData->upgrades;
	
    //normalize stats
    _player.stats.speed = 64 + gameData->upgrades.speed * 2;
	_player.maxHp = 100 + _player.stats.life * 10;
	_player.hp = _player.maxHp;

	_player.hitbox.position = { float(level.mapSize[0]) / 2, float(level.mapSize[1]) / 2 };
	_player.hitbox.size = { 9,13 };

	_player.spritePosition = { _player.hitbox.position[0] - 1, _player.hitbox.position[1] - 2 };
	_player.sprite.size = { 9,13 };
	_player.sprite.texId = 0;
	_player.sprite.texPos = { 0, 0 };
	_player.sprite.texSize = { 9 / 256.0f, 13 / 256.0f };

	level.player = _player;
    return;
}

void levelManager_t::initLevel() {
	level.mapSize = { 4000, 4000 };
	initPlayer();
    cameraPos = level.player.spritePosition;

#if 0
    addSpell(gameData->spellMap[SPELL_TYPE_WHIP]);
    upgradeSpell(*getSpell(SPELL_TYPE_WHIP).value());
    upgradeSpell(*getSpell(SPELL_TYPE_WHIP).value());
    upgradeSpell(*getSpell(SPELL_TYPE_WHIP).value());
    upgradeSpell(*getSpell(SPELL_TYPE_WHIP).value());
#endif

	enemiesKilled = 0;
    enemyCount = 5;
    goldEarned = 0;

	levelTime = 0.0;

	return;
}

void levelManager_t::clearLevel(void) {
	level.enemies.clear();
	level.enemyProjectiles.clear();
	level.playerProjectiles.clear();
	level.pickups.clear();
	level.obstacles.clear();

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
	auto& playerDir = _level.playerDir;
	auto& playerFacingRight = _level.level.player.isTurnedRight;
	auto& currLevel_ = _level.level;
	auto& playerPos = currLevel_.player.hitbox.position;
	float speed = currLevel_.player.stats.speed;

	if (playerDir[0] != 0)
		playerFacingRight = playerDir[0] > 0;

	// dividing by 1.4 to normalize
	if (playerDir[0] && playerDir[1]) speed /= 1.4;
	currLevel_.player.move({ float(speed * playerDir[0] * _dt),
										  float(speed * playerDir[1] * _dt), });

	playerPos[0] = std::clamp(playerPos[0], 0.0f, (float)currLevel_.mapSize[0]);
	playerPos[1] = std::clamp(playerPos[1], 0.0f, (float)currLevel_.mapSize[1]);
	currLevel_.player.spritePosition = playerPos;

	return;
}

inline void updateSpells(levelManager_t& _level, double _dt) {
	auto ats = _level.level.player.stats.attackSpeed;
    
    for (auto& spell : _level.level.player.spells) {
		spell.time -= _dt;
		if (spell.time <= 0) {
			spell.time += spell.cooldownTime;
			spell.effect(_level, spell);
            spell.time *= 1.0f - (0.05f * ats);
        }
	}
	return;
}

inline void updateProjectiles(levelManager_t& _level, double _dt) {
	auto& currLevel_ = _level.level;
	for (size_t i = 0; i < currLevel_.playerProjectiles.size(); i++) {
		auto& proj = currLevel_.playerProjectiles[i];

		proj.lifetime -= _dt;
		if (proj.lifetime <= 0 || 
			(proj.isHitDependent && proj.hits == 0)) {
			currLevel_.playerProjectiles.erase(currLevel_.playerProjectiles.begin() + i);
			i--;
			continue;
		}

		proj.move({ float(_dt * proj.velocity[0]),
						  float(_dt * proj.velocity[1]) });
	}

	return;
}

void levelManager_t::spawnEnemies() {
	auto& playerPos = level.player.hitbox.position;
	auto& enemyData = gameManager->gameData_.enemyData;

	while (level.enemies.size() < enemyCount) {
		static constexpr uint16_t spawnRange = 250;
		double phi = (rand() / double(RAND_MAX)) * 2 * 3.14159265;

		enemy_t enemy{};
		enemy.hitboxPosition = { float(playerPos[0] + spawnRange * sin(phi)), float(playerPos[1] + spawnRange * cos(phi)) };
		//int adsf = rand() % ENEMY_TYPE_MAX;

		int roll = rand() % 10;
		int typeLevel = levelTime / (3 * 60) + 1;
		enemy.type = typeLevel;
		if (roll >= 8) 
			enemy.type++;

		enemy.type = std::min((int)enemy.type, ENEMY_TYPE_MAX-1);
		
		enemy.hp = enemyData[enemy.type].stats.maxHp;
		enemy.isTurnedRight = true;
		enemy.invincibilityTime = 0;

		hitbox_t hitbox = hitbox_t{ .position = enemy.hitboxPosition, .size = enemyData[enemy.type].hitboxSize };
		for (size_t i = 0; i < level.enemies.size(); i++) {
			auto& otherEnemy = level.enemies[i];
			if (checkForIntersection(hitbox, hitbox_t{ .position = otherEnemy.hitboxPosition, .size = enemyData[otherEnemy.type].hitboxSize })) {
				phi = (rand() / double(RAND_MAX)) * 2 * 3.14159265;
				enemy.hitboxPosition = { float(playerPos[0] + (spawnRange + 100) * sin(phi)), float(playerPos[1] + (spawnRange + 100) * cos(phi)) };
				hitbox.position = enemy.hitboxPosition;
				i = 0;
			}
		}
		enemy.spritePosition = enemy.hitboxPosition;
		level.enemies.push_back(enemy);
	}


	return;
}

void levelManager_t::updateEnemies(double _dt) {
	auto& playerPos = level.player.hitbox.position;
	auto& enemyData = gameManager->gameData_.enemyData;
	const auto playerStats = level.player.stats;

	for (size_t i = 0; i < level.enemies.size(); i++) {
		auto& enemy = level.enemies[i];
		auto& enemyStats = enemyData[enemy.type].stats;
		auto enemyHitbox = hitbox_t{ .position = enemy.hitboxPosition, .size = enemyData[enemy.type].hitboxSize };
		auto enemySpeed = enemyData[enemy.type].stats.speed;

		position_t moveDir = {
			_dt* enemySpeed, _dt*enemySpeed
		};
		
		if (enemy.hitboxPosition[0] > playerPos[0])
			moveDir[0] *= -1;
		if (enemy.hitboxPosition[1] > playerPos[1])
			moveDir[1] *= -1;
		
		if(moveDir[0] != 0)
			enemy.isTurnedRight = moveDir[0] > 0;
		
		position_t newPosition = enemy.hitboxPosition;
		newPosition[0] += moveDir[0]; newPosition[1] += moveDir[1];
		hitbox_t newHitbox = hitbox_t{ 
			.position = newPosition, 
			.size = enemyHitbox.size 
		};
		bool x_blocked = false, y_blocked = false;
		
		enemy.invincibilityTime -= _dt;
		if (enemy.invincibilityTime <= 0) {
			for (auto& proj : level.playerProjectiles) {
				bool isIntersecting = checkForIntersection(proj.hitbox, enemyHitbox);
				if (isIntersecting) {
					if (proj.isHitDependent) {
						if (proj.hits == 0) continue;
						proj.hits--;
					}

					//dmg calculation
					float ratio = 1.0f + (playerStats.attack - enemyStats.defense) / (1.0f + enemyStats.defense);
					enemy.hp -= proj.dmg * ratio;

					if (enemy.hp <= 0) {
						//todo: enemy.onDieEffect()
						enemiesKilled++;

						pickup_t dropPickup = {};
						dropPickup.position = enemy.spritePosition;
						dropPickup.position[0] += enemyData[enemy.type].sprite.size[0] * 0.5;
						dropPickup.position[1] += enemyData[enemy.type].sprite.size[1] * 0.5;

						uint8_t roll = rand() % 100;
						if (roll > 80) dropPickup.type = PICKUP_TYPE_GREATER_EXP;
						else dropPickup.type = PICKUP_TYPE_EXP;
						level.pickups.push_back(dropPickup);
                        
                        if(roll < (20+level.player.stats.luck) ) {
                            dropPickup.position[0] += gameData->pickupData[dropPickup.type].sprite.size[0] * 0.5;
                            dropPickup.type = PICKUP_TYPE_GOLD;
                            level.pickups.push_back(dropPickup);
                        }						

                        level.enemies.erase(level.enemies.begin() + i);
						i--;
						goto flag_loop1end;
					}
					enemy.invincibilityTime = 0.25;
				}	// if
			}	// for
		}	// if

		if (checkForIntersection(level.player.hitbox, enemyHitbox)) {
			if (level.player.invincibilityTimeLeft <= 0.0f) {
				//player.onHit();
				level.player.hp -= std::max(
                        enemyData.at(enemy.type).stats.attack - level.player.stats.defense, 
                        0);
				
				level.player.invincibilityTimeLeft = level.player.invincibilityTime;
			}
			goto flag_loop1end;
		}

		for (size_t j = 0; j < level.enemies.size(); j++) {
			auto& otherEnemy = level.enemies[j];
			if (j == i) continue;
			auto otherEnemyHitbox = hitbox_t{ .position = otherEnemy.hitboxPosition, .size = enemyData[otherEnemy.type].hitboxSize };
			if (checkForIntersection(otherEnemyHitbox, newHitbox)) {
				newHitbox.position[1] -= moveDir[1];
				x_blocked = checkForIntersection(otherEnemyHitbox, newHitbox);
				newHitbox.position[1] += moveDir[1];

				newHitbox.position[0] -= moveDir[0];
				y_blocked = checkForIntersection(otherEnemyHitbox, newHitbox);
				newHitbox.position[0] += moveDir[0];

				if(x_blocked && y_blocked)
					break;
			}
		}
		if (x_blocked) moveDir[0] *= -0.1f;
		if (y_blocked) moveDir[1] *= -0.1f;
		enemy.move(moveDir);
		
	flag_loop1end:
		position_t playerDist = { playerPos[0] - enemy.hitboxPosition[0], playerPos[1] - enemy.hitboxPosition[1] };
		float distToPlayer = playerDist[0] * playerDist[0] + playerDist[1] * playerDist[1];
		if (distToPlayer < closestDist) {
			closestDir = { -playerDist[0], -playerDist[1] };
			closestDist = distToPlayer;
		}
	}

	spawnEnemies();

	return;
}

inline bool checkForIntersection(const hitbox_t& _a, const position_t& _b) {
	if (_b[0] >= _a.position[0] && _b[0] <= (_a.position[0] + _a.size[0]))
		if (_b[1] >= _a.position[1] && _b[1] <= (_a.position[1] + _a.size[1]))
			return true;

	return false;
}

inline void updatePickups(levelManager_t& _level) {
	auto& player = _level.level.player;
	for (size_t i = 0; i < _level.level.pickups.size(); i++) {
		auto& pickup = _level.level.pickups[i];
		if (checkForIntersection(player.hitbox, hitbox_t{pickup.position, _level.gameData->pickupData[pickup.type].sprite.size})) {
			switch (pickup.type) {
				//TODO: 
                case PICKUP_TYPE_EXP: {
					player.exp += 20;
				} break;
				case PICKUP_TYPE_GREATER_EXP: {
					player.exp += 50;
				} break;
                case PICKUP_TYPE_GOLD: {
                    _level.goldEarned += 20;
                    _level.gameData->gold += 20;
                } break;
			}

			if (player.exp >= player.expTillNextLvl) {
				player.exp -= player.expTillNextLvl;
				player.lvl += 1;
				player.expTillNextLvl *= 1.5;

				_level.gameManager->currLayout_ = LAYOUT_TYPE_SPELL_CHOICE;
				_level.gameManager->layouts_[_level.gameManager->currLayout_].initLayout(*_level.gameManager);
			}

			_level.level.pickups.erase(_level.level.pickups.begin() + i);
			i--;
			continue;
		}
	}	
	return;
}

inline void updateCamera(levelManager_t& _mng, double _dt) {
	auto& playerPos = _mng.level.player.hitbox.position;
	auto& cameraPos = _mng.cameraPos;

	cameraPos[0] += (playerPos[0] - cameraPos[0]) * 0.9 * _dt;
	cameraPos[1] += (playerPos[1] - cameraPos[1]) * 0.9 * _dt;

	if (abs(cameraPos[0] - playerPos[0]) < 0.01f)
		cameraPos[0] = playerPos[0];
	if (abs(cameraPos[1] - playerPos[1]) < 0.01f)
		cameraPos[1] = playerPos[1];

	return;
}

void levelManager_t::updateGameState(double _dt) {
	if (level.player.hp <= 0) {
		gameManager->currLayout_ = LAYOUT_TYPE_GAME_OVER;
		gameManager->getCurrLayout().initLayout(*gameManager);
        return;
	}
	
	_dt /= 1000; // _dt = 1s
	closestDist = INFINITY;
	level.player.invincibilityTimeLeft -= _dt;
	levelTime += _dt;
	enemyCount = 5 + int(levelTime / 15.0);
	if (playerDir[0] != 0) lastPlayerDir = { playerDir[0], 0 };
	if (playerDir[1] != 0) lastPlayerDir = { 0, playerDir[1] };

	updatePlayerPosition(*this, _dt);
	updateCamera(*this, _dt);
	updateEnemies(_dt);
	updateSpells(*this, _dt);
	updateProjectiles(*this, _dt);
	updatePickups(*this);

	return;
}
