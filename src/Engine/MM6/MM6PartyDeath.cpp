#include "Engine/MM6/MM6PartyDeath.h"

#include "Engine/Objects/Character.h"
#include "Engine/Party.h"
#include "Engine/Spells/SpellBuff.h"

namespace mm6 {

void applyPartyDeath(Party *party, const std::function<int(const Character &)> &maxHealth) {
    party->pHirelings.fill(NPCData());
    ++party->uNumDeaths;
    party->days_played_without_rest = 0;
    party->GetPlayingTime() += Duration::fromDays(7);
    party->last_regenerated = party->GetPlayingTime(); // No regeneration over the skipped week.
    party->uFlags &= ~PARTY_FLAG_WATER_DAMAGE;

    const SpellBuff guardianAngel = party->pPartyBuffs[static_cast<PartyBuff>(8)]; // MM6 buffs keep their original slots, 8 is Guardian Angel.
    party->SetGold(guardianAngel.Active() ? party->uNumGold / 2 : 0, true);
    party->pPartyBuffs.fill(SpellBuff());
    for (Character &character : party->pCharacters) {
        character.conditions.resetAll();
        character.pCharacterBuffs.fill(SpellBuff());
        character.health = 1;
        if (guardianAngel.Active() && guardianAngel.skillMastery == MASTERY_EXPERT)
            character.health = maxHealth(character) / 2;
        else if (guardianAngel.Active() && guardianAngel.skillMastery == MASTERY_MASTER)
            character.health = maxHealth(character);
    }
    party->setActiveCharacterIndex(0);

    party->pos = Vec3f(-9728, -11319, 160);
    party->uFallStartZ = party->pos.z;
    party->_viewYaw = 512;
    party->_viewPitch = 0;
    party->velocity = Vec3f();
}

} // namespace mm6
