#include "saveFiles.hpp"
#include "appData.hpp"

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
    //TODO: RETHINK AND REWRITE THIS
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(__saveData_t, gold, upgrades.attack, upgrades.attackSpeed, 
            upgrades.speed, upgrades.defense, upgrades.luck)
};

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
    __saveData_t sData {};
    try {
        sFile >> saveJson;

        std::print(std::cout, "Game version: {}.{}\n", 
                saveJson.at("versionMaj").get<uint32_t>(),
                saveJson.at("versionMin").get<uint32_t>());

        sData = saveJson; 
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
    
    saveJson = __saveData_t(_data);
    saveJson["versionMaj"] = gameVersionMaj_c;
    saveJson["versionMin"] = gameVersionMin_c;

    sFile << saveJson;
    return;
}
