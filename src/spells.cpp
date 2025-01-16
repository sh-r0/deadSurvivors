#include "spells.hpp"
#include "levelManager.hpp"
#include "gameManager.hpp"
#include <iostream>

#include <unordered_map>

void _whipEffect(levelManager_t& _mng, spell_t& _spell) {
	projectile_t whipStrike{};
	auto& playerHitbox = _mng.level_.player.hitbox;
	whipStrike.hitbox.position = { playerHitbox.position[0] + playerHitbox.size[0], playerHitbox.position[1] };
	whipStrike.isHitDependent = false;
	whipStrike.hitbox.size = { 29, 9 };
	whipStrike.lifetime = 0.1;
	whipStrike.dmg = (50 + _spell.level * 50) * (1 + _spell.level * 0.1);
	whipStrike.hitbox.size[0] *= 1 + (0.1 * _spell.level);

	whipStrike.spritePosition = whipStrike.hitbox.position;
	whipStrike.sprite = _mng.gameData_->projectileData[PROJECTILE_TYPE_WHIP];
	whipStrike.sprite.size = whipStrike.hitbox.size;

	if (!(_mng.level_.player.isTurnedRight)) {
		whipStrike.sprite.texPos[0] += whipStrike.sprite.texSize[0];
		whipStrike.sprite.texSize[0] = -whipStrike.sprite.texSize[0];
		whipStrike.move({ -1.0f * (whipStrike.hitbox.size[0] + playerHitbox.size[0]) , 0 });
	}
	whipStrike.sprite.texId = 0;
	whipStrike.velocity = {};

	_mng.level_.playerProjectiles.push_back(whipStrike);

	return;
}

void _whipEffect2(levelManager_t& _mng, spell_t& _spell) {
	projectile_t whipStrike{};
	auto& playerHitbox = _mng.level_.player.hitbox;
	whipStrike.hitbox.position = { playerHitbox.position[0] + playerHitbox.size[0], playerHitbox.position[1] };
	whipStrike.isHitDependent = false;
	whipStrike.hitbox.size = { 32, 16 };
	whipStrike.lifetime = 0.1;
	whipStrike.dmg = (50 + _spell.level * 50) * (1 + _spell.level * 0.1);
	whipStrike.hitbox.size[0] *= 1 + (0.1 * _spell.level);

	whipStrike.sprite = _mng.gameManager_->gameData_.projectileData[PROJECTILE_TYPE_WHIP];
	whipStrike.sprite.size = whipStrike.hitbox.size;

	whipStrike.sprite.texId = 0;
	whipStrike.velocity = {};

	auto whipStrike2 = whipStrike;
	whipStrike2.sprite.texPos[0] += whipStrike.sprite.texSize[0];
	whipStrike2.sprite.texSize[0] = -whipStrike.sprite.texSize[0];
	whipStrike2.move({ -1.0f * (whipStrike.hitbox.size[0] + playerHitbox.size[0]) , 0 });

	_mng.level_.playerProjectiles.push_back(whipStrike);
	_mng.level_.playerProjectiles.push_back(whipStrike2);

	return;
}

void _swordEffect(levelManager_t& _mng, spell_t& _spell) {
	projectile_t sweep{};
	auto& playerHitbox = _mng.level_.player.hitbox;
	auto playerDir = _mng.lastPlayerDir_;
	sweep.hitbox.position = playerHitbox.position;
	sweep.isHitDependent = false;
	sweep.hitbox.size = { 11, 27 };
	sweep.lifetime = 1.0;
	sweep.dmg = 40 + 20 * _spell.level;

	sweep.spritePosition = sweep.hitbox.position;
	sweep.sprite = _mng.gameData_->projectileData[PROJECTILE_TYPE_SWORD_Y];
	sweep.sprite.size = sweep.hitbox.size;
	//sweep.sprite.texPos = { 34 / 64.0f, 18 / 64.0f };
	//sweep.sprite.texSize = { 11 / 64.0f, 27 / 64.0f };
	//sweep.sprite.texId = 0;
	sweep.velocity = { 0,0 };

	sweep.sprite.size[0] *= 1 + 0.05 * _spell.level;
	sweep.sprite.size[1] *= 1 + 0.05 * _spell.level;

	
	if (playerDir[0] > 0) {
		sweep.hitbox.position[0] += _mng.level_.player.hitbox.size[0] + 2.0f;
	}
	else if (playerDir[0] < 0) {
		sweep.hitbox.position[0] -= sweep.hitbox.size[0] + 2.0f;
		sweep.sprite.texPos[0] += sweep.sprite.texSize[0];
		sweep.sprite.texSize[0] *= -1;
	}
	else if (playerDir[1] > 0) {
		std::swap(sweep.hitbox.size[0], sweep.hitbox.size[1]);
		std::swap(sweep.sprite.size[0], sweep.sprite.size[1]);
		std::swap(sweep.sprite.texSize[0], sweep.sprite.texSize[1]);
		sweep.sprite.texPos = _mng.gameData_->projectileData[PROJECTILE_TYPE_SWORD_X].texPos;

		sweep.hitbox.position[1] += _mng.level_.player.hitbox.size[1] + 2.0f;
		sweep.sprite.texPos[1] += sweep.sprite.texSize[1];
		sweep.sprite.texSize[1] *= -1;
	}
	else {
		std::swap(sweep.hitbox.size[0], sweep.hitbox.size[1]);
		sweep.sprite = _mng.gameData_->projectileData[PROJECTILE_TYPE_SWORD_X];
		sweep.sprite.size = sweep.hitbox.size;

		sweep.hitbox.position[1] -= sweep.hitbox.size[1] + 2.0f;
	}
	sweep.spritePosition = sweep.hitbox.position;

	_mng.level_.playerProjectiles.push_back(sweep);

	return;
}

void _swordEffect2(levelManager_t& _mng, spell_t& _spell) {
	projectile_t sweep{};
	auto& playerHitbox = _mng.level_.player.hitbox;
	auto playerDir = _mng.lastPlayerDir_;
	sweep.hitbox.position = playerHitbox.position;
	sweep.isHitDependent = false;
	sweep.hitbox.size = { 11, 27 };
	sweep.lifetime = 1.0;
	sweep.dmg = 40 + 20 * _spell.level;

	sweep.spritePosition = sweep.hitbox.position;
	sweep.sprite = _mng.gameData_->projectileData[PROJECTILE_TYPE_SWORD_Y];
	sweep.sprite.size = sweep.hitbox.size;
	//sweep.sprite.texPos = { 34 / 64.0f, 18 / 64.0f };
	//sweep.sprite.texSize = { 11 / 64.0f, 27 / 64.0f };
	//sweep.sprite.texId = 0;
	sweep.velocity = { 0,0 };

	sweep.sprite.size[0] *= 1 + 0.05 * _spell.level;
	sweep.sprite.size[1] *= 1 + 0.05 * _spell.level;

	constexpr float sweepVelocity_c = 128.0f;
	if (playerDir[0] > 0) {
		sweep.hitbox.position[0] += _mng.level_.player.hitbox.size[0] + 2.0f;

		sweep.velocity[0] = sweepVelocity_c;
	}
	else if (playerDir[0] < 0) {
		sweep.hitbox.position[0] -= sweep.hitbox.size[0] + 2.0f;
		sweep.sprite.texPos[0] += sweep.sprite.texSize[0];
		sweep.sprite.texSize[0] *= -1;

		sweep.velocity[0] = -sweepVelocity_c;
	}
	else if (playerDir[1] > 0) {
		std::swap(sweep.hitbox.size[0], sweep.hitbox.size[1]);
		std::swap(sweep.sprite.size[0], sweep.sprite.size[1]);
		std::swap(sweep.sprite.texSize[0], sweep.sprite.texSize[1]);
		sweep.sprite.texPos = _mng.gameData_->projectileData[PROJECTILE_TYPE_SWORD_X].texPos;

		sweep.hitbox.position[1] += _mng.level_.player.hitbox.size[1] + 2.0f;
		sweep.sprite.texPos[1] += sweep.sprite.texSize[1];
		sweep.sprite.texSize[1] *= -1;

		sweep.velocity[1] = sweepVelocity_c;
	}
	else {
		std::swap(sweep.hitbox.size[0], sweep.hitbox.size[1]);
		sweep.sprite = _mng.gameData_->projectileData[PROJECTILE_TYPE_SWORD_X];
		sweep.sprite.size = sweep.hitbox.size;

		sweep.hitbox.position[1] -= sweep.hitbox.size[1] + 2.0f;
		sweep.velocity[1] = -sweepVelocity_c;
	}
	sweep.spritePosition = sweep.hitbox.position;

	_mng.level_.playerProjectiles.push_back(sweep);

	return;
}

void _bowEffect(levelManager_t& _mng, spell_t& _spell) {
	projectile_t arrow{};
	auto& playerHitbox = _mng.level_.player.hitbox;
	arrow.isHitDependent = true;
	arrow.hits = 1 + _spell.level / 2;
	arrow.lifetime = 10;
	arrow.hitbox.position = { playerHitbox.position[0] + playerHitbox.size[0] / 2.0f, playerHitbox.position[1] + playerHitbox.size[1] / 2 };
	arrow.hitbox.size = { 10, 3 };
	arrow.dmg = (20 + 5 * (_spell.level + 1) / 2) * (1 + (_spell.level / 2) * 0.05);

	arrow.spritePosition = arrow.hitbox.position;
	arrow.sprite = _mng.gameData_->projectileData[PROJECTILE_TYPE_ARROW];
	arrow.sprite.size = arrow.hitbox.size;
	arrow.velocity = { 128, 0 };

	if (!(_mng.level_.player.isTurnedRight)) {
		arrow.sprite.texPos = { arrow.sprite.texPos[0] + arrow.sprite.texSize[0], arrow.sprite.texPos[1] + arrow.sprite.texSize[1] };
		arrow.sprite.texSize = { -arrow.sprite.texSize[0], -arrow.sprite.texSize[1] };
		arrow.velocity = { -arrow.velocity[0], -arrow.velocity[1] };
		arrow.move({ -1.0f * (arrow.hitbox.size[0]) , 0 });
	}

	_mng.level_.playerProjectiles.push_back(arrow);

	return;
}

void _bowEffect2(levelManager_t& _mng, spell_t& _spell) {
	projectile_t arrow{};
	auto& playerHitbox = _mng.level_.player.hitbox;
	arrow.isHitDependent = true;
	arrow.hits = 1 + _spell.level / 2;
	arrow.lifetime = 10;
	arrow.hitbox.position = { playerHitbox.position[0], playerHitbox.position[1] };
	arrow.hitbox.size = { 10, 3 };
	arrow.dmg = (20 + 5 * (_spell.level + 1) / 2) * (1 + (_spell.level / 2) * 0.05);

	arrow.spritePosition = arrow.hitbox.position;
	arrow.sprite = _mng.gameData_->projectileData[PROJECTILE_TYPE_ARROW];
	arrow.sprite.size = arrow.hitbox.size;
	arrow.velocity = { 128, 0 };

	if (!(_mng.level_.player.isTurnedRight)) {
		arrow.sprite.texPos = { arrow.sprite.texPos[0] + arrow.sprite.texSize[0], arrow.sprite.texPos[1] + arrow.sprite.texSize[1] };
		arrow.sprite.texSize = { -arrow.sprite.texSize[0], -arrow.sprite.texSize[1] };
		arrow.velocity = { -arrow.velocity[0], -arrow.velocity[1] };
		arrow.move({ -1.0f * (arrow.hitbox.size[0]) , 0 });
	}

	_mng.level_.playerProjectiles.push_back(arrow);
	arrow.move({ 0, playerHitbox.size[1] / 3.0f });
	_mng.level_.playerProjectiles.push_back(arrow);
	arrow.move({ 0, playerHitbox.size[1] / 3.0f });
	_mng.level_.playerProjectiles.push_back(arrow);

	return;
}

void _fireOrbEffect(levelManager_t& _mng, spell_t& _spell) {
	if (_mng.closestDist_ == INFINITY) {
		// todo reset timer;
		return;
	}
	float flameSpeed = 128; // speed per frame

	projectile_t flame{};
	auto& playerHitbox = _mng.level_.player.hitbox;
	flame.isHitDependent = true;
	flame.hits = 1 + _spell.level / 5;
	flame.hitbox.position = { playerHitbox.position[0] + _mng.level_.player.hitbox.size[0] / 2.0f, playerHitbox.position[1] };
	flame.hitbox.size = { 8, 8 };
	flame.lifetime = 5;
	flame.dmg = (50 + _spell.level * 5);

	flame.spritePosition = flame.hitbox.position;
	flame.sprite = _mng.gameData_->projectileData[PROJECTILE_TYPE_FLAME];
	flame.sprite.size = flame.hitbox.size;

	flame.velocity = _mng.closestDir_;
	
	normalizeVec(flame.velocity);
	flame.velocity[0] *= flameSpeed;
	flame.velocity[1] *= flameSpeed;

	_mng.level_.playerProjectiles.push_back(flame);

	return;
}

void _fireOrbEffect2(levelManager_t& _mng, spell_t& _spell) {
	_spell.cooldownTime = 5.0;	//todo move it out

	constexpr float flameSpeed = 192/1.4f; // speed per frame

	projectile_t flame{};
	auto& playerHitbox = _mng.level_.player.hitbox;
	flame.isHitDependent = false;
	flame.hitbox.position = { playerHitbox.position[0] + playerHitbox.size[0] / 2.0f, playerHitbox.position[1] + playerHitbox.size[1] / 2.0f};
	flame.hitbox.size = { 8, 8 };
	flame.lifetime = 0.75;
	flame.dmg = (50 + _spell.level * 5);

	flame.spritePosition = flame.hitbox.position;
	flame.sprite = _mng.gameData_->projectileData[PROJECTILE_TYPE_FLAME];
	flame.sprite.size = flame.hitbox.size;

	const static std::vector<position_t> dirs = {
		{-1,-1}, {-1, 0}, {0, -1}, {1,-1}, {-1,1}, {1,0}, {0,1}, {1,1}
	};

	for (auto dir : dirs) {
		flame.velocity = { dir[0] * flameSpeed, dir[1] * flameSpeed };
		if (dir[0] == 0) flame.velocity[1] *= 1.4f;
		else if (dir[1] == 0) flame.velocity[0] *= 1.4f;

		_mng.level_.playerProjectiles.push_back(flame);
	}

	return;
}

using _mp = std::unordered_map<uint8_t, void*>;
inline std::array<std::unordered_map<uint8_t, void*>, SPELL_TYPE_MAX> lvlBreakdowns = {
	_mp{ {4, _whipEffect2} },
	_mp{ {5, _bowEffect2} },
	_mp{ {6, _swordEffect2} },
	_mp{ {7, _fireOrbEffect2} },

};

void upgradeSpell(spell_t& _spell) {
	_spell.level++;

	auto& bdowns = lvlBreakdowns[_spell.type];
	if (bdowns.find(_spell.level) != bdowns.end())
		_spell.effect = (effect_t)bdowns[_spell.level];

	return;
}
