#include "MM6PostMapGeneratedNpcLinks.h"

#include <stdexcept>
#include <utility>

#include "Engine/Objects/MonsterEnumFunctions.h"

namespace mm6 {

size_t requirePostMapGeneratedNpcLinks(std::span<const Actor> actors,
                                       const NPCStats &npcs, GameTitle title) {
    if (title != GameTitle::MM6 || npcs.uNewlNPCBufPos < 0 ||
        static_cast<size_t>(npcs.uNewlNPCBufPos) > npcs.pAdditionalNPC.size())
        throw std::runtime_error("MM6 post-map generated NPC owner is invalid");

    size_t generated = 0;
    for (size_t index = 0; index < actors.size(); ++index) {
        const Actor &actor = actors[index];
        const MonsterId monster = actor.monsterInfo.id;
        if (actor.id != index || monster < MONSTER_FIRST || monster > MONSTER_LAST ||
            actor.npcId < 0)
            throw std::runtime_error("MM6 post-map actor identity is invalid");

        // These two cases are left untouched by the map-load generator.
        if (monsterTierForMonsterId(monster) == MONSTER_TIER_A ||
            (actor.npcId > 0 && actor.npcId < 5000)) {
            if (actor.npcId >= 5000)
                throw std::runtime_error("MM6 weak actor has a generated NPC link");
            continue;
        }

        if (!isPeasant(monster)) {
            if (actor.npcId != 0)
                throw std::runtime_error("MM6 non-peasant retained a generated NPC link");
            continue;
        }

        if (generated >= npcs.pAdditionalNPC.size() ||
            actor.npcId != 5000 + generated)
            throw std::runtime_error("MM6 generated NPC actor links are not contiguous");
        const NPCData &row = npcs.pAdditionalNPC[generated];
        if (row.name.empty() || row.portraitId == 0 || !row.canJoin ||
            row.field_24 != 1 || row.house != HOUSE_INVALID ||
            row.sex != sexForMonsterType(monsterTypeForMonsterId(monster)) ||
            row.profession < NPC_PROFESSION_FIRST ||
            row.profession >= NPC_PROFESSION_MM6_LAST)
            throw std::runtime_error("MM6 generated NPC row is incomplete");
        ++generated;
    }
    if (generated != static_cast<size_t>(npcs.uNewlNPCBufPos))
        throw std::runtime_error("MM6 generated NPC cursor differs from actor links");
    return generated;
}

} // namespace mm6
