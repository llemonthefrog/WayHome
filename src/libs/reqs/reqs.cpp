#include "reqs.h"

#include "cpr/cpr.h"
#include "nlohmann/json.hpp"
#include "iostream"
#include "memory"
#include "sstream"

#include "../parser/parser.h"

using json = nlohmann::json;
using namespace std::chrono;
using namespace cpr;
using std::to_string;
using std::string;

const string yndx_url_nearest = "https://api.rasp.yandex.net/v3.0/nearest_settlement/";
const string yndx_search_url = "https://api.rasp.yandex.net/v3.0/search";
const string geo_url = "https://catalog.api.2gis.com/3.0/items/geocode";

const auto throw_if_error = [](Response& resp) {
    if(resp.status_code != 200) {
        throw std::runtime_error("error in request to server\n" + to_string(resp.status_code) + "\n" + resp.error.message + "\n");
    }
};

const auto get_coords = [](const std::string& cityName) {
    auto resp =  Get(
        Url{ geo_url },
        Parameters{
            {"key", getApiKey("GEO_API")}, {"q", cityName},
            {"fields", "items.point"}
        }
    );

    throw_if_error(resp);
    return resp;
};

const auto get_city_code = [](std::pair<float, float> coords) {
    auto resp = Get(
        Url{ yndx_url_nearest }, 
        Parameters{
            {"apikey", getApiKey("YNDX_API")}, {"lat", to_string(coords.second)}, 
            {"lng", to_string(coords.first)}, {"distance", "1"}
        }
    );

    throw_if_error(resp);
    return resp;
};

const auto search = [](const std::string& code1, const std::string& code2, const std::string& date) {
    auto resp = Get(
        Url( yndx_search_url ),
        Parameters{
            {"apikey", getApiKey("YNDX_API")},
            {"from", code1},
            {"to", code2},
            {"transfers", "true"},
            {"date", date}
        }
    );

    throw_if_error(resp);
    return resp;
};

template <typename T>
void assignIfNotNull(const nlohmann::json& obj, const std::string& key, T& target) {
    if (obj.contains(key) && !obj[key].is_null()) {
        target = obj[key].get<T>();
    }
}

std::string parseDateTime(const std::string& datetime) {
    string result;

    size_t t_pos = datetime.find('T');
    result += (datetime.substr(0, t_pos) + " ");
    string time_and_tz = datetime.substr(t_pos + 1);

    size_t tz_pos = time_and_tz.find_first_of("+-", 0);

    result += (time_and_tz.substr(0, tz_pos) + " ");
    result += "GMT" + time_and_tz.substr(tz_pos);

    return result;
}

void Wire::set(const nlohmann::json& obj, bool dateIsPassed) {
    assignIfNotNull(obj.value("departure_from", json{}), "title", stationFromTytle);
    assignIfNotNull(obj.value("arrival_to", json{}), "title", stationToTytle);

    if (obj.contains("details") && obj["details"].is_array() && !obj["details"].empty()) {
        const auto& firstDetail = obj["details"][0];

        assignIfNotNull(firstDetail.value("thread", json{}), "vehicle", vehicle);
        assignIfNotNull(firstDetail.value("thread", json{}), "transport_type", vehicleType);
        assignIfNotNull(firstDetail.value("thread", json{}).value("carrier", json{}), "title", company);
    } else {
        assignIfNotNull(obj.value("from", json{}), "title", stationFromTytle);
        assignIfNotNull(obj.value("to", json{}), "title", stationToTytle);
        assignIfNotNull(obj.value("thread", json{}), "vehicle", vehicle);
        assignIfNotNull(obj.value("thread", json{}), "transport_type", vehicleType);
        assignIfNotNull(obj.value("thread", json{}).value("carrier", json{}), "title", company);
    }

    if(dateIsPassed) {
        std::string departureTime, arrivalTime;
        assignIfNotNull(obj, "departure", departureTime);
        assignIfNotNull(obj, "arrival", arrivalTime);

        time = parseDateTime(departureTime) + " -> " + parseDateTime(arrivalTime);
    } else {
        std::string departureTime, arrivalTime, startDate;
        assignIfNotNull(obj, "departure", departureTime);
        assignIfNotNull(obj, "arrival", arrivalTime);
        assignIfNotNull(obj, "start_date", startDate);
            
        time = startDate + " " + departureTime + " -> " + arrivalTime;
    }

    if (obj.contains("has_transfers") && obj["has_transfers"] && obj.contains("details")) {
        int count = 0;
        Transfers tr;
        for (const auto& transfer : obj["details"]) {
            if (!transfer.contains("thread") || !transfer["thread"].is_object()) continue;

            if (transfer["thread"].contains("title")) {
                tr.cities += transfer["thread"]["title"].get<std::string>() + " -> ";
            }

            if (transfer["thread"]["carrier"].contains("title")) {
                tr.companies += transfer["thread"]["carrier"]["title"].get<std::string>() + " -> ";
            }

            if (transfer["thread"].contains("vehicle") && transfer["thread"].contains("transport_type")) {
                tr.transport += transfer["thread"]["transport_type"].get<std::string>() + " ";
                tr.transport += transfer["thread"]["vehicle"].get<std::string>() + " -> ";
            }
            count++;
        }
        transfers = std::move(tr);
        MoreThanOne = count > 2 ? true : false;
    }
}

std::string CacheCodes::GetCode(const std::string& name, Config& cfg) {
    if(jsonCodes.contains(name)) {
        return jsonCodes[name];
    } 
        
    Response coords_resp = get_coords(name);
    json coords_data = json::parse(coords_resp.text);
    auto coords = std::pair<float, float>(coords_data["result"]["items"][0]["point"]["lon"], coords_data["result"]["items"][0]["point"]["lat"]);

    Response resp = get_city_code(coords);
    json city_code = json::parse(resp.text);

    jsonCodes[name] = city_code["code"];
    return city_code["code"];
}

void WayHome::getWires(const std::string& city1, const std::string& city2) {
    json data;

    std::string cityCode1 = codes.GetCode(city1, cfg);
    std::string cityCode2 = codes.GetCode(city2, cfg);

    if (!cache.isDateExpired(cityCode1, cityCode2)) {
        const json& dump = cache.readFromFile(cityCode1, cityCode2);

        data = dump["data"];
    } else {
        Response resp = search(cityCode1, cityCode2, cfg.date);
        
        data = json::parse(resp.text);
    }

    std::vector<Wire> wires;
    for (const auto& obj : data["interval_segments"]) {
        Wire elem;
        elem.set(obj, cfg.date.empty()? false : true);
        if(elem.MoreThanOne) {
            wires.push_back(elem);  
        }
    }

    for (const auto& obj : data["segments"]) {
        Wire elem;
        elem.set(obj, cfg.date.empty()? false : true);
        if(!elem.MoreThanOne) {
            wires.push_back(elem);  
        }
    }

    cache.wires = wires;

    if (cache.isExist(cityCode1, cityCode2)) {
        return;
    }

    json wrapped;
    wrapped["data"] = data;
    wrapped["time"] = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    cache.writeInFile(wrapped, cityCode1, cityCode2);
}

void WayHome::print() {
    std::cout << "-------------------\n";
    for (const auto& obj : cache.wires) {
        std::cout << "Станции: " << obj.stationFromTytle << "  ->  " << obj.stationToTytle << "\n";
        if (obj.transfers.cities.empty()) {
            std::cout << "Транспорт: " << obj.vehicleType << " " << obj.vehicle << "\n";
        }
        if (obj.transfers.cities.empty()) {
            std::cout << "Компания: " << obj.company << "\n";
        }
        std::cout << "Дата: " << obj.time << "\n";

        if (!obj.transfers.cities.empty()) {
            std::cout << "Подробнее:\n";
            std::cout << "  -Пункты: " << obj.transfers.cities.substr(0, obj.transfers.cities.size() - 4) << "\n";
            std::cout << "  -Компании: " << obj.transfers.companies.substr(0, obj.transfers.companies.size() - 4) << "\n";
            std::cout << "  -Транспорт: " << obj.transfers.transport.substr(0, obj.transfers.transport.size() - 4)  << "\n";
        } else {
            std::cout << "Без пересадок.\n";
        }

        std::cout << "-------------------\n";
    }
}
