#include "MM6PartyRest.h"

#include "Engine/Graphics/LocationFunctions.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Monsters.h"
#include "Engine/Party.h"

#include "Utility/ScopedRollback.h"

void mm6::restAndHealDetachedParty(Party &party) {
    ScopedRollback<Party *> rollback(&pParty, &party); // Party::restAndHeal and the character stat getters read the party through pParty.
    party.restAndHeal();
}

bool mm6::hostileMonstersNearParty() {
    const int range = uCurrentlyLoadedLevelType == LEVEL_INDOOR ? 2560 : 5120;
    for (const Actor &actor : pActors) {
        if (actor.aiState == Dying || actor.aiState == Dead || actor.aiState == Removed)
            continue;
        // MM6 reads the actor's own hostility byte, which starts out as its monsters.txt hostility. The engine starts
        // every monster friendly until it notices the party.
        const bool hostile = actor.monsterInfo.hostilityType != HOSTILITY_FRIENDLY ||
                             pMonsterStats->infos[actor.monsterInfo.id].hostilityType != HOSTILITY_FRIENDLY;
        if (hostile && (actor.pos.toInt() - pParty->pos.toInt()).octagonalLength() < range)
            return true;
    }
    return false;
}
