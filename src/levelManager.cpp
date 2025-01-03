#include "gameManager.hpp"
#include "levelManager.hpp"

#include <format>
#include <iostream>
#include <stdio.h>
#include <optional>

void levelManager_t::addSpell(spell_t& _spell) {
	level_.player.spellIndexes.insert({ _spell.type, level_.player.spells.size() });
	level_.player.spells.push_back(_spell);

	return;
}

std::optional<spell_t*> levelManager_t::getSpell(spellType _spellType) {
	if (level_.player.spellIndexes.find(_spellType) == level_.player.spellIndexes.end())
		return std::nullopt;

	uint8_t index = level_.player.spellIndexes.at(_spellType);
	return &level_.player.spells[index];
}

void levelManager_t::initLevel() {
	playerInfo_t _player{};
	level_.mapSize = { 4000, 4000 };
	_player.stats = {};
	_player.stats.speed = 64;
	_player.hitbox.position = { float(level_.mapSize[0] / 2), float(level_.mapSize[1] / 2) };
	_player.hitbox.size = { 16,16 };

	_player.spritePosition = _player.hitbox.position;
	_player.sprite.size = { 16,16 };
	_player.sprite.texId = 0;
	_player.sprite.texPos = { 0, 0 };
	_player.sprite.texSize = { 0.25, 0.25 };

	_player.maxHp = 100;
	_player.hp = 100;
	level_.player = _player;
	cameraPos_ = _player.spritePosition;

	enemiesKilled_ = 0;
	enemyCount_ = 5;
	levelTime_ = 0.0;

	//addSpell(gameManager_->gameData_.spellMap.at(SPELL_TYPE_WHIP));
	//addSpell(gameManager_->gameData_.spellMap.at(SPELL_TYPE_BOW));
	//addSpell(gameManager_->gameData_.spellMap.at(SPELL_TYPE_FIRE_ORB));
	//addSpell(gameManager_->gameData_.spellMap.at(SPELL_TYPE_SWORD));

	return;
}

void levelManager_t::clearLevel(void) {
	level_.enemies.clear();
	level_.enemyProjectiles.clear();
	level_.playerProjectiles.clear();
	level_.pickups.clear();
	level_.obstacles.clear();

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
	auto& playerFacingRight = _level.level_.player.isTurnedRight;
	auto& currLevel_ = _level.level_;
	auto& playerPos = currLevel_.player.hitbox.position;

	if (playerDir[0] != 0)
		playerFacingRight = playerDir[0] > 0;

	currLevel_.player.move({ float(currLevel_.player.stats.speed * playerDir[0] * _dt),
										  float(currLevel_.player.stats.speed * playerDir[1] * _dt), });

	playerPos[0] = std::clamp(playerPos[0], 0.0f, (float)currLevel_.mapSize[0]);
	playerPos[1] = std::clamp(playerPos[1], 0.0f, (float)currLevel_.mapSize[1]);
	currLevel_.player.spritePosition = playerPos;

	return;
}

inline void updateSpells(levelManager_t& _level, double _dt) {
	for (auto& spell : _level.level_.player.spells) {
		spell.time -= _dt;
		if (spell.time <= 0) {
			spell.time += spell.cooldownTime;
			spell.effect(_level, spell);
		}
	}
	return;
}

inline void updateProjectiles(levelManager_t& _level, double _dt) {
	auto& currLevel_ = _level.level_;
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
	auto& currLevel_ = _level.level_;
	auto& playerPos = currLevel_.player.hitbox.position;
	auto& enemyData = _level.gameManager_->gameData_.enemyData;

	for (size_t i = 0; i < currLevel_.enemies.size(); i++) {
		auto& enemy = currLevel_.enemies[i];
		auto enemyHitbox = hitbox_t{ .position = enemy.hitboxPosition, .size = enemyData[enemy.type].hitboxSize };
		auto enemySpeed = enemyData[enemy.type].stats.speed;
		position_t moveDir = {
			enemySpeed * _dt, enemySpeed * _dt
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
			for (auto& proj : currLevel_.playerProjectiles) {
				bool isIntersecting = checkForIntersection(proj.hitbox, enemyHitbox);
				if (isIntersecting) {
					enemy.hp -= proj.dmg;
					if (enemy.hp <= 0) {
						//todo: enemy.onDieEffect()
						_level.enemiesKilled_++;

						pickup_t dropPickup = {};
						dropPickup.position = enemy.spritePosition;
						dropPickup.position[0] += enemyData[enemy.type].sprite.size[0] * 0.5;
						dropPickup.position[1] += enemyData[enemy.type].sprite.size[1] * 0.5;

						uint8_t roll = rand() % 10;
						if (roll > 8) dropPickup.type = PICKUP_TYPE_GREATER_EXP;
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

		if (checkForIntersection(currLevel_.player.hitbox, enemyHitbox)) {
			if (currLevel_.player.invincibilityTime <= 0.0f) {
				//player.onHit();
				currLevel_.player.hp -= enemyData.at(enemy.type).stats.attack;
				
				currLevel_.player.invincibilityTime = currLevel_.player.invincibilityTimeShield;
			}
			goto flag_loop1end;
		}

		for (size_t j = 0; j < currLevel_.enemies.size(); j++) {
			auto& otherEnemy = currLevel_.enemies[j];
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
		if (x_blocked) moveDir[0] = 0;
		if (y_blocked) moveDir[1] = 0;
		enemy.move(moveDir);
		
	flag_loop1end:
		position_t playerDist = { playerPos[0] - enemy.hitboxPosition[0], playerPos[1] - enemy.hitboxPosition[1] };
		float distToPlayer = playerDist[0] * playerDist[0] + playerDist[1] * playerDist[1];
		if (distToPlayer < _level.closestDist_) {
			_level.closestDir_ = { -playerDist[0], -playerDist[1] };
			_level.closestDist_ = distToPlayer;
		}

		__noop;
	}

	while (currLevel_.enemies.size() < _level.enemyCount_) {
		static constexpr uint16_t spawnRange = 250;
		double phi = (rand() / double(RAND_MAX)) * 2 * 3.14159265;

		enemy_t enemy{};
		enemy.hitboxPosition = { float(playerPos[0] + spawnRange * sin(phi)), float(playerPos[1] + spawnRange * cos(phi)) };
		enemy.type = ENEMY_TYPE_RED_WALKER;
		enemy.hp = 100;
		enemy.isTurnedRight = true;
		enemy.invincibilityTime = 0;

		hitbox_t hitbox = hitbox_t{ .position = enemy.hitboxPosition, .size = enemyData[enemy.type].hitboxSize };
		for (size_t i = 0; i < currLevel_.enemies.size(); i++) {
			auto& otherEnemy = currLevel_.enemies[i];
			if (checkForIntersection(hitbox, hitbox_t{ .position = otherEnemy.hitboxPosition, .size = enemyData[otherEnemy.type].hitboxSize })) {
				phi = (rand() / double(RAND_MAX)) * 2 * 3.14159265;
				enemy.hitboxPosition = { float(playerPos[0] + (spawnRange+100) * sin(phi)), float(playerPos[1] + (spawnRange+100) * cos(phi)) };
				hitbox.position = enemy.hitboxPosition;
				i = 0;
			}
		}
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

inline void updatePickups(levelManager_t& _level) {
	auto& player = _level.level_.player;
	for (size_t i = 0; i < _level.level_.pickups.size(); i++) {
		auto& pickup = _level.level_.pickups[i];
		if (checkForIntersection(player.hitbox, pickup.position)) {
			switch (pickup.type) {
			case PICKUP_TYPE_EXP: {
				player.exp += 20;
			} break;
			case PICKUP_TYPE_GREATER_EXP: {
				player.gold += 50;
			} break;
			}

			if (player.exp >= player.expTillNextLvl) {
				player.exp -= player.expTillNextLvl;
				player.lvl += 1;
				player.expTillNextLvl *= 1.5;

				_level.gameManager_->currLayout_ = LAYOUT_TYPE_SPELL_CHOICE;
				_level.gameManager_->layouts_[_level.gameManager_->currLayout_].initLayout(*_level.gameManager_);
			}

			_level.level_.pickups.erase(_level.level_.pickups.begin() + i);
			i--;
			continue;
		}
	}
	return;
}

inline void updateCamera(levelManager_t& _mng, double _dt) {
	auto& playerPos = _mng.level_.player.hitbox.position;
	auto& cameraPos = _mng.cameraPos_;

	cameraPos[0] += (playerPos[0] - cameraPos[0]) * 0.9 * _dt;
	cameraPos[1] += (playerPos[1] - cameraPos[1]) * 0.9 * _dt;

	if (abs(cameraPos[0] - playerPos[0]) < 0.01f)
		cameraPos[0] = playerPos[0];
	if (abs(cameraPos[1] - playerPos[1]) < 0.01f)
		cameraPos[1] = playerPos[1];

	return;
}

void levelManager_t::updateGameState(double _dt) {
	if (level_.player.hp <= 0) {
		gameManager_->currLayout_ = LAYOUT_TYPE_GAME_OVER;
		return;
	}
	
	_dt /= 1000; // _dt = 1s
	closestDist_ = INFINITY;
	level_.player.invincibilityTime -= _dt;
	levelTime_ += _dt;
	enemyCount_ = 5 + int(levelTime_ / 30.0);
	if (playerDir_[0] != 0) lastPlayerDir_ = { playerDir_[0], 0 };
	if (playerDir_[1] != 0) lastPlayerDir_ = { 0, playerDir_[1] };

	updatePlayerPosition(*this, _dt);
	updateCamera(*this, _dt);
	updateEnemies(*this, _dt);
	updateSpells(*this, _dt);
	updateProjectiles(*this, _dt);
	updatePickups(*this);

	return;
}