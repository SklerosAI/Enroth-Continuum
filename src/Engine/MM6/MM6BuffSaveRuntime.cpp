#include "MM6BuffSaveFields.h"

#include <cstddef>
#include <stdexcept>

#include "Engine/Party.h"
#include "Engine/Spells/SpellBuff.h"

namespace mm6 {
namespace {

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 timed buffs cannot apply to another title");
}

SavedSpellBuff capture(const SpellBuff &buff) {
    if (buff.caster < -1 || buff.caster > 3)
        throw std::runtime_error("MM6 buff caster exceeds save range");
    return {buff.expireTime.ticks(), buff.power, static_cast<std::uint8_t>(buff.skillMastery),
            buff.overlayId, static_cast<std::int8_t>(buff.caster), buff.isGM};
}

void apply(const SavedSpellBuff &saved, SpellBuff *buff) {
    buff->expireTime = Time::fromTicks(saved.expireTicks);
    buff->power = saved.power;
    buff->skillMastery = static_cast<Mastery>(saved.mastery);
    buff->overlayId = saved.overlayId;
    buff->caster = saved.caster;
    buff->isGM = saved.isGM;
}

} // namespace

BuffSaveFields captureBuffFields(const Party &party, GameTitle title) {
    requireMM6(title);
    BuffSaveFields fields;
    for (size_t index = 0; index < fields.party.size(); ++index)
        fields.party[index] = capture(party.pPartyBuffs[static_cast<PartyBuff>(index)]);
    for (size_t player = 0; player < fields.characters.size(); ++player)
        for (size_t index = 0; index < fields.characters[player].size(); ++index)
            fields.characters[player][index] = capture(
                party.pCharacters[player].pCharacterBuffs[static_cast<CharacterBuff>(index)]);
    (void) encodeBuffSection(fields);
    return fields;
}

Party stageBuffParty(const Party &live, const BuffSaveFields &fields, GameTitle title) {
    requireMM6(title);
    (void) encodeBuffSection(fields); // Validate every decoded field before copying the Party.
    Party staged = live;
    for (size_t index = 0; index < fields.party.size(); ++index)
        apply(fields.party[index], &staged.pPartyBuffs[static_cast<PartyBuff>(index)]);
    for (size_t player = 0; player < fields.characters.size(); ++player)
        for (size_t index = 0; index < fields.characters[player].size(); ++index)
            apply(fields.characters[player][index],
                  &staged.pCharacters[player].pCharacterBuffs[static_cast<CharacterBuff>(index)]);
    return staged;
}

Party stageBuffParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title) {
    requireMM6(title);
    BuffSaveFields fields;
    loadBuffFieldsBeforeMutation(bytes, &fields);
    return stageBuffParty(live, fields, title);
}

} // namespace mm6
