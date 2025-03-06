#pragma once

#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <optional>

#include "gameTypes.hpp"

struct saveData_t {
    uint32_t gold;

    playerStats_t upgrades;
};

std::optional<saveData_t> loadSaveData(void);
void saveSaveData(const saveData_t& _data);
