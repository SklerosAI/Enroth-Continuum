#include "MM6TransportRules.h"

#include <algorithm>
#include <cassert>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include "Engine/Resources/EngineFileSystem.h"

#include "Library/Json/Json.h"

#include "Utility/Memory/Blob.h"

namespace mm6 {
namespace {

int boundedInt(const Json &value, int low, int high) {
    if (!value.is_number_integer())
        throw std::runtime_error("Invalid MM6 transport integer type");
    const int result = value.get<int>();
    if (result < low || result > high)
        throw std::runtime_error("MM6 transport integer outside verified bounds");
    return result;
}

TransportSlotKind slotKind(const std::string &value) {
    if (value == "blank")
        return TransportSlotKind::Blank;
    if (value == "scheduled")
        return TransportSlotKind::Scheduled;
    if (value == "weekday_uncertain")
        return TransportSlotKind::WeekdayUncertain;
    if (value == "special_unresolved")
        return TransportSlotKind::SpecialUnresolved;
    throw std::runtime_error("Unknown MM6 transport slot kind");
}

} // namespace

const TransportHouse *TransportRules::findHouse(int id) const {
    auto it = std::find_if(houses.begin(), houses.end(), [id](const TransportHouse &house) { return house.id == id; });
    return it == houses.end() ? nullptr : &*it;
}

std::array<const TransportRoute *, 3> TransportRules::offeredRoutes(int houseId, int weekday, const std::function<bool(int)> &hasQuestBit) const {
    std::array<const TransportRoute *, 3> result{};
    const TransportHouse *house = findHouse(houseId);
    if (!house)
        return result;
    assert(weekday >= 0 && weekday < 7);
    std::optional<size_t> lastShown;
    for (size_t slot = 0; slot < result.size(); ++slot) {
        const size_t index = house->slots[slot].route;
        const TransportRoute &route = routes[index];
        if (index == lastShown || !route.days[weekday] || (route.questBit && !hasQuestBit(route.questBit)))
            continue;
        result[slot] = &route;
        lastShown = index;
    }
    return result;
}

int transportPrice(int houseId, float value, int merchant) {
    if (houseId == 59)
        return 0;
    const int full = static_cast<int>((isTransportStable(houseId) ? 25 : 50) * static_cast<double>(value));
    return std::max((100 - merchant) * full / 100, full / 3);
}

TransportRules decodeTransportRules(const Blob &source) {
    if (source.size() > 65536)
        throw std::runtime_error("MM6 transport manifest is too large");
    const Json document = Json::parse(source.str());
    if (!document.is_object() || document.at("format") != 1 ||
        !document.at("source_report_sha256").is_string() ||
        document.at("source_report_sha256").get<std::string>().size() != 64)
        throw std::runtime_error("Invalid MM6 transport manifest header");

    const Json &routes = document.at("routes");
    const Json &houses = document.at("houses");
    if (!routes.is_array() || routes.size() != 36 || !houses.is_array() || houses.size() != 21)
        throw std::runtime_error("Invalid MM6 transport manifest shape");

    TransportRules result;
    result.routes.reserve(routes.size());
    for (const Json &row : routes) {
        TransportRoute route;
        route.mapIndex = boundedInt(row.at("map_index"), 1, 4096);
        route.duration = boundedInt(row.at("duration"), 1, 30);
        route.yaw = boundedInt(row.at("yaw"), 0, 2047);
        route.questBit = boundedInt(row.at("quest_bit"), 0, 511);
        const Json &days = row.at("days");
        const Json &position = row.at("position");
        if (!days.is_array() || days.size() != 7 || !position.is_array() || position.size() != 3)
            throw std::runtime_error("Invalid MM6 transport route arrays");
        for (size_t i = 0; i < 7; ++i)
            route.days[i] = boundedInt(days[i], 0, 1);
        for (size_t i = 0; i < 3; ++i)
            route.position[i] = boundedInt(position[i], -32768, 32767);
        result.routes.push_back(route);
    }

    std::unordered_set<int> seenHouses;
    size_t scheduled = 0, uncertain = 0, special = 0;
    result.houses.reserve(houses.size());
    for (const Json &row : houses) {
        TransportHouse house;
        house.id = boundedInt(row.at("id"), 1, 255);
        if (!seenHouses.insert(house.id).second)
            throw std::runtime_error("Duplicate MM6 transport house");
        const Json &slots = row.at("slots");
        if (!slots.is_array() || slots.size() != 3)
            throw std::runtime_error("Invalid MM6 transport house slots");
        for (size_t i = 0; i < 3; ++i) {
            house.slots[i].kind = slotKind(slots[i].at("kind").get<std::string>());
            house.slots[i].route = boundedInt(slots[i].at("route"), 0, 35);
            scheduled += house.slots[i].kind == TransportSlotKind::Scheduled;
            uncertain += house.slots[i].kind == TransportSlotKind::WeekdayUncertain;
            special += house.slots[i].kind == TransportSlotKind::SpecialUnresolved;
        }
        result.houses.push_back(house);
    }
    if (scheduled != 34 || uncertain != 1 || special != 2)
        throw std::runtime_error("MM6 transport slot categories differ from F3 gate");
    return result;
}

const TransportRules &transportRules() {
    static const TransportRules rules = decodeTransportRules(ufs->read("rules/transport.json"));
    return rules;
}

} // namespace mm6
