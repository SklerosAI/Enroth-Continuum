#pragma once

#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Monsters.h"

namespace mm6 {

/**
 * Whether MM6 counts a monster as hostile to the party, for the rest check, the alert gems and the minimap. MM6 reads
 * the actor's own hostility byte, which starts out as its monsters.txt hostility, while the engine starts every monster
 * friendly until it notices the party.
 *
 * @param actor                         Monster to check.
 * @return                              Whether its runtime or its monsters.txt hostility isn't friendly.
 */
inline bool isMonsterHostileToParty(const Actor &actor) {
    return actor.monsterInfo.hostilityType != HOSTILITY_FRIENDLY ||
           pMonsterStats->infos[actor.monsterInfo.id].hostilityType != HOSTILITY_FRIENDLY;
}

} // namespace mm6
