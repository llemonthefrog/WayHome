#pragma once

#include <string>
#include <vector>
#include <cinttypes>
#include <chrono>
#include <iostream>
#include <nlohmann/json.hpp>

#include "../parser/parser.h"

struct Transfers final {
    std::string cities;
    std::string companies;
    std::string transport;
};

struct Wire final {
    std::string stationFromTytle;
    std::string stationToTytle;
    std::string vehicle;
    std::string vehicleType;
    std::string time;
    std::string company;
    std::string stops;

    bool MoreThanOne;
    
    Transfers transfers;

    void set(const nlohmann::json& obj, bool dateIsPassed);
};

class Cache final {
public:
    std::vector<Wire> wires;

    bool isDateExpired(const std::string&, const std::string&);
    bool isExist(const std::string&, const std::string&);
    void writeInFile(const nlohmann::json&, const std::string&, const std::string&);
    nlohmann::json readFromFile(const std::string&, const std::string&);
};

class CacheCodes final {
public:
    nlohmann::json jsonCodes;

    std::string GetCode(const std::string&, Config&);
    void Read(int=0);
    void Write();

    CacheCodes() {
        Read();
    }

    ~CacheCodes() {
        Write();
    }
};

class WayHome final {
public:
    Config cfg;
    Cache cache;
    CacheCodes codes;

    void getWires(const std::string&, const std::string&);
    void print();

    WayHome(char** argc, int argv) {   
        cfg.Parse(argc, argv);    
    }
};