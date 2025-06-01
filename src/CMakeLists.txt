cmake_minimum_required(VERSION 3.20)
project(labwork6)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.8.1
)
FetchContent_MakeAvailable(cpr)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

add_subdirectory(libs)

add_executable(wayHome main.cpp)

target_link_libraries(wayHome PRIVATE 
    cpr::cpr 
    nlohmann_json::nlohmann_json
    Parser
    Reqs
)