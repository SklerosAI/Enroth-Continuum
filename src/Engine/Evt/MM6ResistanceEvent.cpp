#include "Engine/Evt/MM6ResistanceEvent.h"

#include <array>
#include <cstdint>
#include <stdexcept>

#include "Engine/Objects/Character.h"

namespace mm6 {

using ResistanceField = int16_t Character::*;

// MM6 has Fire, Electricity, Cold, Poison and Magic resistances. The shared runtime keeps Electricity, Cold and
// Poison in its Air, Water and Body fields.
static constexpr std::array<ResistanceField, 10> resistanceFields = {
    &Character::sResFireBase, &Character::sResAirBase, &Character::sResWaterBase,
    &Character::sResBodyBase, &Character::sResMagicBase,
    &Character::sResFireBonus, &Character::sResAirBonus, &Character::sResWaterBonus,
    &Character::sResBodyBonus, &Character::sResMagicBonus,
};

static ResistanceField resistanceField(int rawId) {
    if (!isResistanceVariable(rawId))
        throw std::runtime_error("MM6 resistance event variable is invalid");
    return resistanceFields[rawId - 46];
}

bool isResistanceVariable(int rawId) {
    return rawId >= 46 && rawId <= 55;
}

bool compareEventResistance(const Character &character, int rawId, int value) {
    return character.*resistanceField(rawId) >= value;
}

void setEventResistance(Character &character, int rawId, int value) {
    character.*resistanceField(rawId) = static_cast<int16_t>(value & 0xff);
}

void addEventResistance(Character &character, int rawId, int value) {
    int16_t &field = character.*resistanceField(rawId);
    field = static_cast<int16_t>(field + static_cast<int16_t>(value));
    if (field > 255)
        field = 255;
}

void subtractEventResistance(Character &character, int rawId, int value) {
    int16_t &field = character.*resistanceField(rawId);
    field = static_cast<int16_t>(field - static_cast<int16_t>(value));
}

SpeechId eventResistanceReaction(int rawId) {
    if (!isResistanceVariable(rawId))
        throw std::runtime_error("MM6 resistance event variable is invalid");
    return rawId <= 50 ? SPEECH_STAT_BASE_INC : SPEECH_STAT_BONUS_INC;
}

} // namespace mm6
