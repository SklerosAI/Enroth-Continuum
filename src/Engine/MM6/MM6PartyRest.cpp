#include "MM6PartyRest.h"

#include <utility>

#include "Engine/Evt/EvtContext.h"
#include "Engine/Evt/MM6HirelingEvent.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/LocationFunctions.h"
#include "Engine/MM6/MM6PartyHostility.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/Tables/NPCTable.h"

#include "Utility/ScopedRollback.h"

static constexpr QuestBit QBIT_MM6_ENTERTAIN_NICOLAI = static_cast<QuestBit>(90);
static constexpr int NPC_MM6_NICOLAI = 13;
static constexpr int EVENT_MM6_NICOLAI_DISAPPEARS = 20;

static bool partyRested = false;

void mm6::restAndHealDetachedParty(Party &party) {
    ScopedRollback<Party *> rollback(&pParty, &party); // Party::restAndHeal and the character stat getters read the party through pParty.
    party.restAndHeal();
}

bool mm6::hostileMonstersNearParty() {
    const int range = uCurrentlyLoadedLevelType == LEVEL_INDOOR ? 2560 : 5120;
    for (const Actor &actor : pActors) {
        if (actor.aiState == Dying || actor.aiState == Dead || actor.aiState == Removed)
            continue;
        if (isMonsterHostileToParty(actor) && (actor.pos.toInt() - pParty->pos.toInt()).octagonalLength() < range)
            return true;
    }
    return false;
}

void mm6::notePartyRested() {
    partyRested = true;
}

void mm6::runPartyRestedEvent() {
    if (!std::exchange(partyRested, false))
        return;
    if (pParty->_questBits[QBIT_MM6_ENTERTAIN_NICOLAI] && isEventNpcHired(*pNPCStats, NPC_MM6_NICOLAI))
        runEvent(EvtContext(EVT_SOURCE_GLOBAL, EVENT_MM6_NICOLAI_DISAPPEARS));
}
