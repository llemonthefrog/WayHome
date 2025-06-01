#include "reqs.h"

#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;
using namespace std::chrono;

constexpr int timeOut = 3600;

const std::string cacheDir = "/cacheForLab/";
const std::string codesFile = "/cacheForLab/codesCache.json";

#ifdef _WIN32
    const auto homeDir = std::string(std::getenv("USERPROFILE"));
#else
    const auto homeDir = std::string(std::getenv("HOME"));
#endif


void CacheCodes::Read() {
    std::ifstream stream(homeDir + codesFile);

    if (!stream.is_open()) {
        std::cout << "there is no existing cache for codes\nplease, restart the program\n";
        std::ofstream ostream(homeDir + codesFile);
        nlohmann::json jsn;
        jsn["Санкт-Петербург"] = "c2";

        ostream << jsn.dump(4);
        ostream.close();
        exit(0);
    }

    stream >> jsonCodes;

    stream.close();
}

void CacheCodes::Write() {
    std::ofstream stream(homeDir + codesFile);
    if (!stream.is_open()) {
        std::cout << "error in writing file\n";
        return;
    }

    stream << jsonCodes.dump(4);

    stream.close();
}

bool Cache::isExist(const std::string& code1, const std::string& code2) {
    if(!fs::exists(homeDir + cacheDir)) {
        fs::create_directory(homeDir + cacheDir);
    }

    return fs::exists(homeDir + cacheDir + code1 + "-" + code2 + ".json");
}

bool Cache::isDateExpired(const std::string& code1, const std::string& code2) {
    if(!isExist(code1, code2)) {
        return true;
    }

    auto dump = readFromFile(code1, code2);
    uint64_t secs = dump["time"]; 
    time_point lastRequestTime = system_clock::time_point(seconds{secs});

    return system_clock::to_time_t(system_clock::now()) - system_clock::to_time_t(lastRequestTime) >= timeOut;
}

void Cache::writeInFile(const nlohmann::json& js, const std::string& code1, const std::string& code2) {
    std::ofstream stream(homeDir + cacheDir + code1 + "-" + code2 + ".json");

    if (!stream.is_open()) {
        throw std::runtime_error("error in writing file\n");
    }

    stream << js.dump(4);
    stream.close();
}

nlohmann::json Cache::readFromFile(const std::string& code1, const std::string& code2) {
    std::ifstream stream(homeDir + cacheDir + code1 + "-" + code2 + ".json");
    if (!stream.is_open()) {
        throw std::runtime_error("error in reading file\n");
    }

    nlohmann::json obj;

    stream >> obj;
    stream.close();

    return obj;
}
