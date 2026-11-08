#include "MM6PartyRest.h"

#include "Engine/Party.h"

#include "Utility/ScopedRollback.h"

void mm6::restAndHealDetachedParty(Party &party) {
    ScopedRollback<Party *> rollback(&pParty, &party); // Party::restAndHeal and the character stat getters read the party through pParty.
    party.restAndHeal();
}
