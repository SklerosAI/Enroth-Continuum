#pragma once

#include "Engine/Objects/MonsterEnums.h"

namespace mm6 {

// The original MM6 map-load loop only creates a StreetNPC for these three
// native monster IDs. Other actors have their native NPC marker cleared.
// Keep this narrow until the positive StreetNPC namespace is implemented.
constexpr bool nativeStreetNpcEligible(MonsterId monster) {
    const int id = static_cast<int>(monster);
    return id == 40 || id == 41 || id == 44;
}

constexpr bool clearNoneligibleSignedActorNpc(MonsterId monster, int nativeNpcId) {
    return nativeNpcId < 0 && !nativeStreetNpcEligible(monster);
}

} // namespace mm6
