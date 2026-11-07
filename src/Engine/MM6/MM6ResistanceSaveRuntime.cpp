#include "MM6ResistanceSaveFields.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "Engine/Engine.h"
#include "Engine/Party.h"

namespace mm6 {
namespace {

using ResistanceMember = std::int16_t Character::*;

constexpr std::array<ResistanceMember, 11> BASE = {
    &Character::sResFireBase, &Character::sResAirBase, &Character::sResWaterBase,
    &Character::sResEarthBase, &Character::sResPhysicalBase, &Character::sResMagicBase,
    &Character::sResSpiritBase, &Character::sResMindBase, &Character::sResBodyBase,
    &Character::sResLightBase, &Character::sResDarkBase,
};

constexpr std::array<ResistanceMember, 11> BONUS = {
    &Character::sResFireBonus, &Character::sResAirBonus, &Character::sResWaterBonus,
    &Character::sResEarthBonus, &Character::sResPhysicalBonus, &Character::sResMagicBonus,
    &Character::sResSpiritBonus, &Character::sResMindBonus, &Character::sResBodyBonus,
    &Character::sResLightBonus, &Character::sResDarkBonus,
};

void requireMM6(GameTitle title, bool requireEngine) {
    if (title != GameTitle::MM6 || (requireEngine && !engine) ||
        (engine && engine->gameTitle() != GameTitle::MM6))
        throw std::runtime_error("MM6 resistance fields cannot apply to another title");
}

} // namespace

ResistanceSaveFields captureResistanceFields(const Party &party, GameTitle title) {
    requireMM6(title, false);
    ResistanceSaveFields fields;
    for (size_t characterIndex = 0; characterIndex < fields.characters.size(); ++characterIndex) {
        const Character &source = party.pCharacters[characterIndex];
        ResistanceCharacterFields &target = fields.characters[characterIndex];
        for (size_t channel = 0; channel < BASE.size(); ++channel) {
            target.base[channel] = source.*BASE[channel];
            target.bonus[channel] = source.*BONUS[channel];
        }
    }
    return fields;
}

Party stageResistanceParty(const Party &live, const ResistanceSaveFields &fields, GameTitle title) {
    requireMM6(title, false);
    Party staged = live;
    for (size_t characterIndex = 0; characterIndex < fields.characters.size(); ++characterIndex) {
        Character &target = staged.pCharacters[characterIndex];
        const ResistanceCharacterFields &source = fields.characters[characterIndex];
        for (size_t channel = 0; channel < BASE.size(); ++channel) {
            target.*BASE[channel] = source.base[channel];
            target.*BONUS[channel] = source.bonus[channel];
        }
    }
    return staged;
}

Party stageResistanceParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title) {
    requireMM6(title, false);
    ResistanceSaveFields fields;
    loadResistanceFieldsBeforeMutation(bytes, &fields);
    return stageResistanceParty(live, fields, title);
}

} // namespace mm6
