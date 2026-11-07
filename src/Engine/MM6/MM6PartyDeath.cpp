#include "Engine/MM6/MM6PartyDeath.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

#include "Engine/Party.h"
#include "Engine/Objects/Character.h"
#include "Engine/Objects/CharacterConditions.h"
#include "Engine/Spells/SpellBuff.h"

namespace mm6 {
namespace {

bool hasUnmappedBuff(const SpellBuff &buff) {
    return buff.expireTime.ticks() != 0 || buff.power != 0 ||
           buff.skillMastery != MASTERY_NONE || buff.overlayId != 0 ||
           buff.caster != -1 || buff.isGM;
}

} // namespace

PartyDeathResult planPartyDeath(const Party &source, GameTitle title,
                               GuardianAngelState guardianAngel,
                               const std::array<int, 4> &maximumHealth) {
    if (title != GameTitle::MM6)
        return PartyDeathRefusal::ForeignTitle;
    if (guardianAngel == GuardianAngelState::Unknown)
        return PartyDeathRefusal::UnknownGuardianAngel;
    if (source.uNumGold < 0)
        return PartyDeathRefusal::InvalidGold;
    if (source.uNumDeaths == std::numeric_limits<int>::max())
        return PartyDeathRefusal::DeathCountOverflow;

    constexpr std::int64_t weekTicks = Duration::fromDays(7).ticks();
    if (source.playing_time.ticks() < 0 ||
        source.playing_time.ticks() > std::numeric_limits<std::int64_t>::max() - weekTicks)
        return PartyDeathRefusal::PlayingTimeOverflow;

    // The original handler clears 16 MM6 party and character buffs and 18
    // condition timestamps. Runtime tail slots are MM7-sized and not mapped.
    // The first 16 MM6 buff IDs still need a live MM6-to-runtime mapping
    // before a Game.cpp adapter can use this detached plan.
    for (int index = 16; index < 20; ++index)
        if (hasUnmappedBuff(source.pPartyBuffs[static_cast<PartyBuff>(index)]))
            return PartyDeathRefusal::UnmappedBuff;
    for (const Character &character : source.pCharacters) {
        for (int index = 16; index < 24; ++index)
            if (hasUnmappedBuff(character.pCharacterBuffs[static_cast<CharacterBuff>(index)]))
                return PartyDeathRefusal::UnmappedBuff;
        for (int index = 17; index <= 18; ++index)
            if (character.conditions.get(static_cast<Condition>(index)).ticks() != 0)
                return PartyDeathRefusal::UnmappedCondition;
    }
    if (guardianAngel != GuardianAngelState::Absent)
        for (int health : maximumHealth)
            if (health <= 0)
                return PartyDeathRefusal::InvalidMaximumHealth;

    auto staged = std::make_unique<Party>(source);
    ++staged->uNumDeaths;
    staged->playing_time += Duration::fromDays(7);
    staged->uNumGold = guardianAngel == GuardianAngelState::Absent ? 0 : source.uNumGold / 2;
    for (int index = 0; index < 16; ++index)
        staged->pPartyBuffs[static_cast<PartyBuff>(index)] = SpellBuff();
    for (int player = 0; player < 4; ++player) {
        Character &character = staged->pCharacters[player];
        for (int index = 0; index < 16; ++index)
            character.pCharacterBuffs[static_cast<CharacterBuff>(index)] = SpellBuff();
        character.conditions.resetAll();
        switch (guardianAngel) {
            case GuardianAngelState::Absent:
                character.health = 1;
                break;
            case GuardianAngelState::Expert:
                character.health = maximumHealth[player] / 2;
                break;
            case GuardianAngelState::Master:
                character.health = maximumHealth[player];
                break;
            case GuardianAngelState::Unknown:
                break; // Rejected before staging.
        }
    }
    staged->pos = Vec3f(-9728, -11319, 160);
    staged->_viewYaw = 512;
    staged->_viewPitch = 0;
    staged->velocity = Vec3f();
    return PartyDeathPlan{std::move(staged)};
}

} // namespace mm6
