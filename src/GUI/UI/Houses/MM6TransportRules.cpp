#include "MM6TransportRules.h"

#include <algorithm>
#include <cassert>
#include <optional>

#include "Engine/MM6/MM6Executable.h"

namespace mm6 {

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
        const size_t index = house->routes[slot];
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

TransportRules decodeTransportRules(const Executable &exe) {
    TransportRules result;
    for (int i = 0; i < 36; i++) {
        const std::uint32_t row = 0x4C3F20 + 32 * i;
        TransportRoute route;
        route.mapIndex = exe.u8(row);
        for (int day = 0; day < 7; day++)
            route.days[day] = exe.u8(row + 1 + day);
        route.duration = exe.u8(row + 8);
        for (int axis = 0; axis < 3; axis++)
            route.position[axis] = exe.i32(row + 12 + 4 * axis);
        route.yaw = exe.i32(row + 24);
        route.questBit = exe.i32(row + 28);
        result.routes.push_back(route);
    }
    for (int houseId = 48; houseId <= 68; houseId++) {
        TransportHouse house;
        house.id = houseId;
        for (int slot = 0; slot < 3; slot++)
            house.routes[slot] = exe.u8(0x4C43A0 + 3 * (houseId - 48) + slot);
        result.houses.push_back(house);
    }
    return result;
}

const TransportRules &transportRules() {
    static const TransportRules rules = decodeTransportRules(executable());
    return rules;
}

} // namespace mm6
