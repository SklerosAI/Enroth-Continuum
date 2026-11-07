#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Monsters.h"

namespace mm6 {

// SpawnEncounter copies the static monster row, including two strings that
// Actor::GetName reads from MonsterStats instead. Snapshot reconstruction
// clears them. Strip only an exact static-table copy in a detached save image;
// modified or partially copied names are not representable by ACTR/ARTR.
inline std::vector<Actor> canonicalActorsWithDerivedNamesRemoved(
    std::span<const Actor> live, const MonsterStats &staticMonsters,
    GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 derived actor names require MM6 title");
    std::vector<Actor> result(live.begin(), live.end());
    for (std::size_t slot = 0; slot < result.size(); ++slot) {
        Actor &actor = result[slot];
        if (actor.monsterInfo.name.empty() && actor.monsterInfo.internalName.empty())
            continue;
        bool validId = false;
        for (MonsterId id : staticMonsters.infos.indices())
            if (id == actor.monsterInfo.id) {
                validId = true;
                break;
            }
        if (!validId)
            throw std::runtime_error("MM6 actor derived name has invalid monster ID at slot " +
                                     std::to_string(slot));
        const MonsterInfo &source = staticMonsters.infos[actor.monsterInfo.id];
        if (actor.monsterInfo.name != source.name ||
            actor.monsterInfo.internalName != source.internalName)
            throw std::runtime_error("MM6 actor runtime name differs from static row at slot " +
                                     std::to_string(slot));
        actor.monsterInfo.name.clear();
        actor.monsterInfo.internalName.clear();
    }
    return result;
}

} // namespace mm6
