add_library(Reqs reqs.cpp cust_cache.cpp)

target_include_directories(Reqs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Reqs PRIVATE cpr::cpr nlohmann_json::nlohmann_json)
target_link_libraries(Reqs PUBLIC Parser)
