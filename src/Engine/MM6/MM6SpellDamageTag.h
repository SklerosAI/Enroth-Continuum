#pragma once

#include <stdexcept>
#include <string_view>

#include "Utility/String/Ascii.h"

namespace mm6 {

// The seven source labels in MM6 spells.txt are not MM7 spell schools.
enum class SpellDamageTag { None, Physical, Fire, Electricity, Cold, Poison, Magic };

inline SpellDamageTag parseSpellDamageTag(std::string_view label) {
    if (ascii::noCaseEquals(label, "none")) return SpellDamageTag::None;
    if (ascii::noCaseEquals(label, "Phys")) return SpellDamageTag::Physical;
    if (ascii::noCaseEquals(label, "Fire")) return SpellDamageTag::Fire;
    if (ascii::noCaseEquals(label, "Elec")) return SpellDamageTag::Electricity;
    if (ascii::noCaseEquals(label, "Cold")) return SpellDamageTag::Cold;
    if (ascii::noCaseEquals(label, "Pois")) return SpellDamageTag::Poison;
    if (ascii::noCaseEquals(label, "Magic")) return SpellDamageTag::Magic;
    throw std::runtime_error("Unknown MM6 spell damage tag");
}

}  // namespace mm6
