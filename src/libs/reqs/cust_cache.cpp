#include "reqs.h"

#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;
using namespace std::chrono;
using nlohmann::json;
using std::string;

constexpr int timeOut = 3600;

const string cacheDir = "/cacheForLab/";
const string codesFile = "/cacheForLab/codesCache.json";

#ifdef _WIN32
    const auto homeDir = string(std::getenv("USERPROFILE"));
#else
    const auto homeDir = string(std::getenv("HOME"));
#endif

void CacheCodes::Read(int tr) {
    if(tr >= 16) {
        throw std::runtime_error("number of attempts exceeded");
    }

    std::ifstream stream(homeDir + codesFile);

    if (!stream.is_open()) {
        std::cout << "There is no existing cache for codes\ncreating...\n";

        if(!fs::exists(homeDir + cacheDir)) {
            fs::create_directory(homeDir + cacheDir);
        }

        std::ofstream ostream(homeDir + codesFile);
        json jsn;
        jsn["Санкт-Петербург"] = "c2";

        ostream << jsn.dump(4);
        ostream.close();
        Read(tr+1);
        return;
    }

    if (stream.peek() == std::ifstream::traits_type::eof()) {
        std::cout << "Cache is empty, writing cache...\n";
        json jsn;
        jsn["Санкт-Петербург"] = "c2";
        stream.close();

        std::ofstream ostream(homeDir + codesFile);
        ostream << jsn.dump(4);
        ostream.close();
        Read(tr+1);
        return;
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

bool Cache::isExist(const string& code1, const string& code2) {
    if(!fs::exists(homeDir + cacheDir)) {
        fs::create_directory(homeDir + cacheDir);
    }

    return fs::exists(homeDir + cacheDir + code1 + "-" + code2 + ".json");
}

bool Cache::isDateExpired(const string& code1, const string& code2) {
    if(!isExist(code1, code2)) {
        return true;
    }

    auto dump = readFromFile(code1, code2);
    uint64_t secs = dump["time"]; 
    time_point lastRequestTime = system_clock::time_point(seconds{secs});

    return system_clock::to_time_t(system_clock::now()) - system_clock::to_time_t(lastRequestTime) >= timeOut;
}

void Cache::writeInFile(const json& js, const string& code1, const string& code2) {
    std::ofstream stream(homeDir + cacheDir + code1 + "-" + code2 + ".json");

    if (!stream.is_open()) {
        throw std::runtime_error("error in writing file\n");
    }

    stream << js.dump(4);
    stream.close();
}

json Cache::readFromFile(const string& code1, const string& code2) {
    std::ifstream stream(homeDir + cacheDir + code1 + "-" + code2 + ".json");
    if (!stream.is_open()) {
        throw std::runtime_error("error in reading file\n");
    }

    json obj;

    stream >> obj;
    stream.close();

    return obj;
}
