#pragma once

#include <array>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "Engine/Spells/SpellEnums.h"
#include "Utility/String/Ascii.h"

namespace mm6 {

// Active MM6 monsters.txt spell names map to MM6's 99-ID vocabulary.
// Some names differ from the inherited enum; two distinct names contain
// source spelling errors.
inline SpellId monsterSpellId(std::string_view name) {
    constexpr std::array<std::pair<std::string_view, int>, 20> names = {{
        {"Cold Beam", 24},
        {"Deadly Swarm", 37},
        {"Dispell Magic", 80},
        {"Finger of Death", 95},
        {"Fire Bolt", 4},
        {"Fireball", 6},
        {"Flame Arrow", 2},
        {"Flying Fist", 76},
        {"Harm", 70},
        {"Ice Bolt", 28},
        {"Incinerate", 11},
        {"Lightning Bolt", 18},
        {"Mass Curse", 91},
        {"Meteor Shower", 9},
        {"Mind Blast", 58},
        {"Paralyze", 86},
        {"Poison Spray", 26},
        {"Psychic Shock", 65},
        {"Psychic Shockt", 65},
        {"Toxic Cloud", 90},
    }};
    for (const auto &[label, id] : names)
        if (ascii::noCaseEquals(name, label))
            return static_cast<SpellId>(id);
    throw std::runtime_error("Unknown active MM6 monster spell name");
}

}  // namespace mm6
