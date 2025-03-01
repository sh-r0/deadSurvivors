#pragma once

#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <optional>

struct saveData_t {
    uint32_t gold;


};

std::optional<saveData_t> loadSaveData(void);
void saveSaveData(const saveData_t& _data);
