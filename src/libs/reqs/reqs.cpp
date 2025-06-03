#include "reqs.h"

#include "cpr/cpr.h"
#include "nlohmann/json.hpp"
#include "iostream"
#include "memory"
#include "sstream"
#include <unordered_map>

#include "../parser/parser.h"

using json = nlohmann::json;
using namespace std::chrono;
using namespace cpr;
using std::to_string;
using std::string;

const std::unordered_map<int, string> err_msgs = {
    {400, "(400) - Bad request"},
    {401, "(401) - Unauthorized"},
    {403, "(403) - Forbiden"},
    {404, "(404)\nMost likely there are no flights at the moment\nTry changing the date or requesting later"}
};

const string yndx_url_nearest = "https://api.rasp.yandex.net/v3.0/nearest_settlement/";
const string yndx_search_url = "https://api.rasp.yandex.net/v3.0/search";
const string geo_url = "https://api.geoapify.com/v1/geocode/search";

const auto throw_if_error = [](Response& resp) {
    if(err_msgs.find(resp.status_code) != err_msgs.end()) {
        std::cerr << err_msgs.at(resp.status_code) << "\n";
        exit(1);
    }

    if(resp.status_code / 100 != 2) {
        throw std::runtime_error("unknown error in requests: " + to_string(resp.status_code));
    }
};

const auto get_coords = [](const string& cityName) -> Response {
    auto resp =  Get(
        Url{ geo_url },
        Parameters{
            {"apiKey", getApiKey("GEO_API")}, {"name", cityName}
        }
    );

    throw_if_error(resp);
    return resp;
};

const auto get_city_code = [](std::pair<float, float> coords) -> Response {
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

const auto search = [](const string& code1, const string& code2, const string& date) -> Response {
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
    size_t t_pos = datetime.find('T');
    if (t_pos == string::npos) {
        return datetime; 
    }

    string result = datetime.substr(0, t_pos) + " ";
    string time_and_tz = datetime.substr(t_pos + 1);

    size_t tz_pos = time_and_tz.find_first_of("+-");
    result += time_and_tz.substr(0, tz_pos) + " ";
    if (tz_pos != string::npos) {
        result += "GMT" + time_and_tz.substr(tz_pos);
    }
    return result;
}

void Wire::setDetails(const json& obj) {
    if (obj.contains("details") && obj["details"].is_array() && !obj["details"].empty()) {
        const auto& firstDetail = obj["details"][0];

        assignIfNotNull(firstDetail.value("thread", json{}), "vehicle", vehicle);
        assignIfNotNull(firstDetail.value("thread", json{}), "transport_type", vehicleType);
        assignIfNotNull(firstDetail.value("thread", json{}).value("carrier", json{}), "title", company);
        return;
    } 

    assignIfNotNull(obj.value("from", json{}), "title", stationFromTytle);
    assignIfNotNull(obj.value("to", json{}), "title", stationToTytle);
    assignIfNotNull(obj.value("thread", json{}), "vehicle", vehicle);
    assignIfNotNull(obj.value("thread", json{}), "transport_type", vehicleType);
    assignIfNotNull(obj.value("thread", json{}).value("carrier", json{}), "title", company);
}

void Wire::setDate(const json& obj, bool dateIsPassed) {
    string departureTime, arrivalTime, startDate;
    assignIfNotNull(obj, "departure", departureTime);
    assignIfNotNull(obj, "arrival", arrivalTime);

    if(dateIsPassed) {
        time = parseDateTime(departureTime) + " -> " + parseDateTime(arrivalTime);
    } else {
        assignIfNotNull(obj, "start_date", startDate);
            
        time = startDate + " " + departureTime + " -> " + arrivalTime;
    }
}

void Wire::setTransfers(const json& obj) {
    if (!obj.contains("has_transfers") || !obj["has_transfers"] || !obj.contains("details")) {
        return;
    }

    int count = 0;
    Transfers tr;
    for (const auto& transfer : obj["details"]) {
        if (!transfer.contains("thread") || !transfer["thread"].is_object()) {
            continue;
        }

        const auto& thr = transfer["thread"];

        if (thr.contains("title")) {
            tr.cities += thr["title"].get<string>() + " -> ";
        }

        if (thr.contains("carrier") && thr["carrier"].is_object() && thr["carrier"].contains("title")) {
            tr.companies += thr["carrier"]["title"].get<string>() + " -> ";
        }

        if (thr.contains("vehicle") && thr.contains("transport_type")) {
            tr.transport += thr["transport_type"].get<string>() + " ";
            tr.transport += thr["vehicle"].get<string>() + " -> ";
        }
        ++count;
    }

    transfers = std::move(tr);
    MoreThanOne = count > 2;
}

void Wire::set(const json& obj, bool dateIsPassed) {
    assignIfNotNull(obj.value("departure_from", json{}), "title", stationFromTytle);
    assignIfNotNull(obj.value("arrival_to", json{}), "title", stationToTytle);

    setDetails(obj);
    setDate(obj, dateIsPassed);
    setTransfers(obj);
}

std::string CacheCodes::GetCode(const std::string& name, Config& cfg) {
    if(jsonCodes.contains(name)) {
        return jsonCodes[name];
    } 
        
    Response coords_resp = get_coords(name);
    json coords_data = json::parse(coords_resp.text);
    auto coords = std::pair<float, float>(coords_data["features"][0]["properties"]["lon"], coords_data["features"][0]["properties"]["lat"]);

    Response resp = get_city_code(coords);
    json city_code = json::parse(resp.text);

    jsonCodes[name] = city_code["code"];
    return city_code["code"];
}

void WayHome::getWires(const std::string& city1, const std::string& city2) {
    json data;

    string cityCode1 = codes.GetCode(city1, cfg);
    string cityCode2 = codes.GetCode(city2, cfg);

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
        elem.set(obj, !cfg.date.empty());
        if(elem.MoreThanOne) {
            wires.push_back(elem);  
        }
    }

    for (const auto& obj : data["segments"]) {
        Wire elem;
        elem.set(obj, !cfg.date.empty());
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
    const string line = "-------------------\n";

    std::cout << line;
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

        std::cout << line;
    }
}
