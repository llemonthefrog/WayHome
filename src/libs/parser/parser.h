#pragma once

#include <string>

class Config final {
public:
    std::string date;
    std::string cityFrom;
    std::string cityTo;

    std::string getApiKey(const std::string&);

    void Parse(char**, int);

    Config() : date(""), cityFrom(""), cityTo("") {}
};
