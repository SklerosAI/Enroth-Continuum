#pragma once

#include <utility>

#include "Engine/MM6/MM6NativeActorNpcPolicy.h"
#include "Engine/Objects/MonsterEnums.h"

namespace mm6 {

/**
 * @offset 0x403050
 *
 * @param reputation                    Party reputation before the monster dies.
 * @param tableHostility                Hostility of the monster's `monsters.txt` row.
 * @param arenaFight                    Whether an Arena fight is on.
 * @return                              Party reputation after the death. Outside the Arena every death of a monster
 *                                      whose row is friendly costs 100, whoever dealt the blow. MM6 has no fines.
 */
constexpr int reputationAfterMonsterDeath(int reputation, MonsterHostility tableHostility, bool arenaFight) {
    return tableHostility == HOSTILITY_FRIENDLY && !arenaFight ? reputation - 100 : reputation;
}

/**
 * @offset 0x403730
 *
 * @param reputation                    Party reputation before the monster is hurt.
 * @param wasHostile                    Whether the monster was hostile before it was hurt.
 * @return                              Party reputation after the hit. Hurting a monster that wasn't hostile costs 50.
 */
constexpr int reputationAfterPain(int reputation, bool wasHostile) {
    return wasHostile ? reputation : reputation - 50;
}

/**
 * @offset 0x432550
 *
 * @param victim                        MM6 monster ID of the monster that was hurt or killed.
 * @param bystander                     MM6 monster ID of a monster within 4096 units of it.
 * @return                              Whether the bystander turns hostile. A hurt peasant of the Peasant F1, F2 or M1
 *                                      families turns the peasants of all three hostile. Any other monster turns
 *                                      only its own family.
 */
constexpr bool bystanderTurnsHostile(MonsterId victim, MonsterId bystander) {
    if (nativeStreetNpcEligible(victim))
        return nativeStreetNpcEligible(bystander);
    return (std::to_underlying(victim) - 1) / 3 == (std::to_underlying(bystander) - 1) / 3;
}

/**
 * @offset 0x4615FB
 *
 * @param monster                       MM6 monster ID.
 * @return                              Whether the monster keeps to water outdoors, the way land monsters keep to land.
 *                                      These are the Water Elemental and Sea Serpent families.
 */
constexpr bool keepsToWater(MonsterId monster) {
    const int id = std::to_underlying(monster);
    return (id >= 58 && id <= 60) || (id >= 151 && id <= 153);
}

} // namespace mm6
