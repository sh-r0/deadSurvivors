#include "utils.hpp"
#include "gameTypes.hpp"
#include "levelManager.hpp"

void _whipEffect(levelManager_t& _mng, spell_t& _spell);
void _swordEffect(levelManager_t& _mng, spell_t& _spell);
void _bowEffect(levelManager_t& _mng, spell_t& _spell);
void _fireOrbEffect(levelManager_t& _mng, spell_t& _spell);

//void _whipEffect2(levelManager_t& _mng, spell_t& _spell);
//void _swordEffect2(levelManager_t& _mng, spell_t& _spell);
//void _bowEffect2(levelManager_t& _mng, spell_t& _spell);
//void _fireOrbEffect2(levelManager_t& _mng, spell_t& _spell);

void upgradeSpell(spell_t& _spell);