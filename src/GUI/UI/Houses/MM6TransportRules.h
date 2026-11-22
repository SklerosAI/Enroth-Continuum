#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace mm6 {

class Executable;

struct TransportRoute {
    size_t mapIndex = 0; // One-based position in the original Games LOD directory.
    std::array<uint8_t, 7> days{};
    int duration = 0;
    std::array<int, 3> position{};
    int yaw = 0;
    int questBit = 0;
};

struct TransportHouse {
    int id = 0;
    std::array<size_t, 3> routes{}; // Route of each slot. Every slot has one, a slot that's blank in 2DEvents.txt too.
};

struct TransportRules {
    std::vector<TransportRoute> routes;
    std::vector<TransportHouse> houses;

    const TransportHouse *findHouse(int id) const;

    /**
     * The original's transport menu. A slot shows its route on the route's weekdays when the route has no quest bit or
     * the party has it, unless the last slot shown has the same route.
     *
     * @param houseId                   MM6 house id.
     * @param weekday                   Day of the month modulo 7.
     * @param hasQuestBit               Whether the party has a quest bit.
     * @return                          Route of each of the house's three slots, `nullptr` for a hidden slot. All
     *                                  `nullptr` for a house without routes.
     */
    std::array<const TransportRoute *, 3> offeredRoutes(int houseId, int weekday, const std::function<bool(int)> &hasQuestBit) const;
};

/**
 * @param houseId                       MM6 transport house id.
 * @return                              Whether the house is a stable. The others are boats.
 */
constexpr bool isTransportStable(int houseId) {
    return houseId < 57;
}

/**
 * @param houseId                       MM6 transport house id.
 * @param value                         The house's `Val` column in 2DEvents.txt.
 * @param merchant                      Character's MM6 merchant percentage.
 * @return                              Gold a trip costs, never below a third of the full price. House 59 is free.
 */
int transportPrice(int houseId, float value, int merchant);

/**
 * @param houseId                       MM6 transport house id.
 * @param farewellMs                    Length of the horseman's or boatman's farewell in milliseconds, zero without
 *                                      one.
 * @return                              Milliseconds between paying and leaving: the farewell's length, but never more
 *                                      than the original's fixed 1500 for a coach and 2500 for a boat, which also
 *                                      apply without a farewell.
 */
int transportDepartureDelayMs(int houseId, int farewellMs);

/**
 * @param exe                           `MM6.exe` to read the route and slot tables from.
 * @return                              The 36 routes and the slots of the 21 stables and boats.
 */
TransportRules decodeTransportRules(const Executable &exe);
const TransportRules &transportRules();

} // namespace mm6
