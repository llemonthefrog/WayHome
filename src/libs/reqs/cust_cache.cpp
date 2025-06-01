#include "reqs.h"

#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

void CacheCodes::Read() {
    using std::string;

    string homeDir;

    #ifdef _WIN32
        homeDir = string(std::getenv("USERPROFILE"));
    #else
        homeDir = string(std::getenv("HOME"));
    #endif

    std::ifstream stream(homeDir + "/cacheForLab/codesCache.json");
    if (!stream.is_open()) {
        std::cout << "there is no existing cache for codes\nplease, restart the program\n";
        std::ofstream ostream(homeDir + "/cacheForLab/codesCache.json");
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
    using std::string;

    string homeDir;

    #ifdef _WIN32
        homeDir = string(std::getenv("USERPROFILE"));
    #else
        homeDir = string(std::getenv("HOME"));
    #endif

    std::ofstream stream(homeDir + "/cacheForLab/codesCache.json");
    if (!stream.is_open()) {
        std::cout << "error in writing file\n";
        return;
    }

    stream << jsonCodes.dump(4);

    stream.close();
}

bool Cache::isExist(const std::string& code1, const std::string& code2) {
    using std::string;

    string homeDir;

    #ifdef _WIN32
        homeDir = string(std::getenv("USERPROFILE"));
    #else
        homeDir = string(std::getenv("HOME"));
    #endif

    if(!fs::exists(homeDir + "/cacheForLab")) {
        fs::create_directory(homeDir + "/cacheForLab");
    }

    return fs::exists(homeDir + "/cacheForLab/" + code1 + "-" + code2 + ".json");
}

bool Cache::isDateExpired(const std::string& code1, const std::string& code2) {
    using namespace std::chrono;

    if(!isExist(code1, code2)) {
        return true;
    }

    nlohmann::json dump = readFromFile(code1, code2);
    uint64_t secs = dump["time"]; 
    time_point lastRequestTime = system_clock::time_point(seconds{secs});

    if(system_clock::to_time_t(system_clock::now()) - system_clock::to_time_t(lastRequestTime) >= 3600) {
        return true;
    } 

    return false;
}

void Cache::writeInFile(const nlohmann::json& js, const std::string& code1, const std::string& code2) {
    using namespace std::chrono;
    using std::string;

    string homeDir;

    #ifdef _WIN32
        homeDir = string(std::getenv("USERPROFILE"));
    #else
        homeDir = string(std::getenv("HOME"));
    #endif

    std::ofstream stream(homeDir + "/cacheForLab/" + code1 + "-" + code2 + ".json");

    if (!stream.is_open()) {
        std::cout << "error in writing file\n";
        return;
    }

    stream << js.dump(4);

    stream.close();
}

nlohmann::json Cache::readFromFile(const std::string& code1, const std::string& code2) {
    using namespace std::chrono;
    using std::string;

    string homeDir;

    #ifdef _WIN32
        homeDir = string(std::getenv("USERPROFILE"));
    #else
        homeDir = string(std::getenv("HOME"));
    #endif

    nlohmann::json obj;

    std::ifstream stream(homeDir + "/cacheForLab/" + code1 + "-" + code2 + ".json");
    if (!stream.is_open()) {
        std::cout << "error in reading file\n";
        return obj;
    }

    stream >> obj;

    stream.close();

    return obj;
}
