#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

class Blob;

namespace mm6 {

enum class TransportSlotKind { Blank, Scheduled, WeekdayUncertain, SpecialUnresolved };

struct TransportRoute {
    size_t mapIndex = 0; // One-based position in the original Games LOD directory.
    std::array<uint8_t, 7> days{};
    int duration = 0;
    std::array<int, 3> position{};
    int yaw = 0;
    int questBit = 0;
};

struct TransportSlot {
    TransportSlotKind kind = TransportSlotKind::Blank;
    size_t route = 0;
};

struct TransportHouse {
    int id = 0;
    std::array<TransportSlot, 3> slots{};
};

struct TransportRules {
    std::vector<TransportRoute> routes;
    std::vector<TransportHouse> houses;

    const TransportHouse *findHouse(int id) const;
    const TransportRoute *scheduledRoute(int houseId, size_t slot, int weekday, bool hasQuestBit) const;
};

// Only a generated private profile manifest may supply the game-specific values.
TransportRules decodeTransportRules(const Blob &source);
const TransportRules &transportRules();

} // namespace mm6
