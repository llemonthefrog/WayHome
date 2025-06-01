#include "parser.h"

#include <iostream>

std::string Config::getApiKey(const std::string& keyName) {
    char* key = std::getenv(keyName.c_str());
    if(key == nullptr) {
        throw std::runtime_error("There is no api key\nPlease, open .bashrc and write ' export" + keyName + "=\"your key\" '\n");
    }

    return std::string(key);
}

void Config::Parse(char** input, int size) {
    for(int i = 1; i < size; i++) {
        std::string elem = std::string(input[i]);
        
        if(elem == "--date") {
            date = std::string(input[i+1]);
            i++;
        } else if(elem == "--from") {
            cityFrom = std::string(input[i+1]);
            i++;
        } else if(elem == "--to") {
            cityTo = std::string(input[i+1]);
            i++;
        } else {
            throw std::runtime_error("unknown key option: " + elem);
        }
    }
}