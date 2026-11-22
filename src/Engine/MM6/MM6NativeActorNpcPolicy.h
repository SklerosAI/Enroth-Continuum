#pragma once

#include <cassert>
#include <utility>

#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/MonsterEnums.h"

namespace mm6 {

/**
 * @param monster                       MM6 monster ID.
 * @return                              Whether the monster belongs to one of the three peasant families that the
 *                                      original map load gives street NPCs to.
 */
constexpr bool nativeStreetNpcEligible(MonsterId monster) {
    const int family = (static_cast<int>(monster) - 1) / 3; // MM6 monster IDs come in families of three tiers.
    return family == 40 || family == 41 || family == 44;
}

/**
 * The original map load gives a street NPC to an eligible actor whose map record has a nonzero NPC link, of any sign.
 *
 * @param monster                       MM6 monster ID.
 * @param npcId                         Actor's NPC link before the map load assigns street NPCs.
 * @return                              Whether the actor gets a street NPC.
 */
constexpr bool getsStreetNpc(MonsterId monster, int npcId) {
    return npcId != 0 && nativeStreetNpcEligible(monster);
}

/**
 * The original map load clears the NPC link of every actor outside the eligible families.
 *
 * @param monster                       MM6 monster ID.
 * @param npcId                         Actor's NPC link before the map load assigns street NPCs.
 * @return                              Whether the map load clears this actor's link.
 */
constexpr bool clearsNativeActorNpc(MonsterId monster, int npcId) {
    return npcId != 0 && !nativeStreetNpcEligible(monster);
}

/**
 * @offset 0x469210
 *
 * @param monster                       MM6 monster ID of an actor that gets a street NPC.
 * @return                              Sex of its street NPC, which picks the name and the portrait. MM6.exe reads a
 *                                      byte per monster ID at 0x4BF8B4, female for the Peasant F1 and F2 families and
 *                                      male for Peasant M1.
 */
constexpr Sex nativeStreetNpcSex(MonsterId monster) {
    assert(nativeStreetNpcEligible(monster));
    return (std::to_underlying(monster) - 1) / 3 == 44 ? SEX_MALE : SEX_FEMALE;
}

} // namespace mm6
