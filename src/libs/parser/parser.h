#pragma once

#include <string>

std::string getApiKey(const std::string&);

class Config final {
public:
    std::string date;
    std::string cityFrom;
    std::string cityTo;

    void Parse(char**, int);
};
