#include "saveFiles.hpp"

#include <istream>
#include <nlohmann/detail/macro_scope.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <optional>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <string>

constexpr std::string saveFilePath_c = "saveData.json";

struct __saveData_t : saveData_t {
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(__saveData_t, gold)
};

inline saveData_t saveFromJson(const nlohmann::json& _json) {
    saveData_t res{};

    res.gold = _json.at("gold").get<uint32_t>();
    
    return res; 
}

std::optional<saveData_t> loadSaveData(void) {
    if(!std::filesystem::exists(saveFilePath_c)) {
        return std::nullopt;
    }

    std::ifstream sFile(saveFilePath_c.c_str());
    if(sFile.fail()) {
        std::cerr<<"couldn't open save file!\n";
        return std::nullopt;
    }

    nlohmann::json saveJson;
    sFile >> saveJson;
    
    saveData_t sData {};
    try {
        sData = saveFromJson(saveJson);
    } catch (const std::exception& _e) { //TODO:: INFORM PLAYER THAT __THIS__ COULDNT BE LOADED
        std::cerr<<_e.what();
        return std::nullopt;
    }

    return sData;
}

void saveSaveData(const saveData_t& _data) {
    std::ofstream sFile(saveFilePath_c.c_str());
    if(sFile.fail()) {
        std::cerr<<"couldn't open save file!\n";
        return;
    }

    nlohmann::json saveJson;
    __saveData_t sData(_data);
    saveJson = sData;

    sFile << saveJson;
    return;
}
