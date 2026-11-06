#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include "Engine/GameTitle.h"
#include "Engine/Objects/ItemEnums.h"
#include "Utility/String/Ascii.h"

namespace mm6 {

// The seven labels in MM6 monsters.txt are independent of MM7 spell schools.
enum class MonsterDamageLabel { Physical, Fire, Electricity, Cold, Poison, Magic, Energy };

inline MonsterDamageLabel parseMonsterDamageLabel(std::string_view label) {
    if (ascii::noCaseEquals(label, "Phys")) return MonsterDamageLabel::Physical;
    if (ascii::noCaseEquals(label, "Fire")) return MonsterDamageLabel::Fire;
    if (ascii::noCaseEquals(label, "Elec")) return MonsterDamageLabel::Electricity;
    if (ascii::noCaseEquals(label, "Cold")) return MonsterDamageLabel::Cold;
    if (ascii::noCaseEquals(label, "Pois")) return MonsterDamageLabel::Poison;
    if (ascii::noCaseEquals(label, "Magic")) return MonsterDamageLabel::Magic;
    if (ascii::noCaseEquals(label, "Ener")) return MonsterDamageLabel::Energy;
    throw std::runtime_error("Unknown active MM6 monster damage label: " + std::string(label));
}

// Explicit adapter for storage in MonsterInfo. The inherited Air/Water/Earth
// slots are provisional aliases; the effect gate below does not enable them.
inline DamageType damageTypeForMonsterLabel(MonsterDamageLabel label) {
    switch (label) {
        case MonsterDamageLabel::Physical: return DAMAGE_PHYSICAL;
        case MonsterDamageLabel::Fire: return DAMAGE_FIRE;
        case MonsterDamageLabel::Electricity: return DAMAGE_AIR;
        case MonsterDamageLabel::Cold: return DAMAGE_WATER;
        case MonsterDamageLabel::Poison: return DAMAGE_EARTH;
        case MonsterDamageLabel::Magic: return DAMAGE_MAGIC;
        case MonsterDamageLabel::Energy: return DAMAGE_ENERGY;
    }
    throw std::runtime_error("Invalid MM6 monster damage label");
}

inline DamageType damageTypeForMonsterAttack(std::string_view label, bool active) {
    // A zero-chance secondary attack has no effect and uses "0" in the table.
    return active ? damageTypeForMonsterLabel(parseMonsterDamageLabel(label)) : DAMAGE_PHYSICAL;
}

// Physical and Fire have existing direct damage paths. Other labels need an
// MM6 character resistance mapping or source-backed Magic/Energy rule first.
inline void requireVerifiedMonsterDamage(GameTitle title, DamageType type) {
    if (title != GameTitle::MM6) return;
    if (type == DAMAGE_PHYSICAL || type == DAMAGE_FIRE) return;
    throw std::runtime_error("MM6 damage channel is not verified");
}

}  // namespace mm6
