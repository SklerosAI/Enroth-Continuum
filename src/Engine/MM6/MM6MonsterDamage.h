#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include "Engine/GameTitle.h"
#include "Engine/Objects/CharacterEnums.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/Monsters.h"
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

// The retained table has six resistance columns. Energy is an attack label,
// but no seventh resistance column exists; its rule needs separate proof.
inline uint8_t monsterResistanceForAttack(const MonsterInfo &monster, MonsterDamageLabel label) {
    if (!monster.mm6Resistances)
        throw std::runtime_error("MM6 monster resistance source is unavailable");
    const MonsterResistances &values = *monster.mm6Resistances;
    switch (label) {
        case MonsterDamageLabel::Physical: return values.physical;
        case MonsterDamageLabel::Fire: return values.fire;
        case MonsterDamageLabel::Electricity: return values.electricity;
        case MonsterDamageLabel::Cold: return values.cold;
        case MonsterDamageLabel::Poison: return values.poison;
        case MonsterDamageLabel::Magic: return values.magic;
        case MonsterDamageLabel::Energy: break;
    }
    throw std::runtime_error("MM6 Energy resistance mapping is unverified");
}

/**
 * Refuses MM6 damage to a monster in a channel whose monster resistance rule is not verified.
 *
 * @param title                         Running game title, everything passes outside MM6.
 * @param type                          Damage channel.
 */
inline void requireVerifiedMonsterDamage(GameTitle title, DamageType type) {
    if (title != GameTitle::MM6) return;
    if (type == DAMAGE_PHYSICAL || type == DAMAGE_FIRE) return;
    throw std::runtime_error("MM6 damage channel is not verified");
}

/**
 * @param type                          Damage channel, the shared runtime stores MM6 Electricity as Air and Cold as
 *                                      Water.
 * @return                              MM6 character resistance that the original damage check reads for it.
 */
inline Attribute characterResistanceForDamage(DamageType type) {
    switch (type) {
        case DAMAGE_FIRE: return ATTRIBUTE_RESIST_FIRE;
        case DAMAGE_AIR: return ATTRIBUTE_RESIST_AIR;
        case DAMAGE_WATER: return ATTRIBUTE_RESIST_WATER;
        default: break;
    }
    throw std::runtime_error("MM6 character resistance for this damage channel is not verified");
}

/**
 * Refuses MM6 damage to a character in a channel whose character resistance rule is not verified.
 *
 * @param title                         Running game title, everything passes outside MM6.
 * @param type                          Damage channel.
 */
inline void requireVerifiedCharacterDamage(GameTitle title, DamageType type) {
    if (title != GameTitle::MM6) return;
    if (type == DAMAGE_PHYSICAL) return; // MM6 applies no resistance or luck roll to physical damage.
    characterResistanceForDamage(type);
}

}  // namespace mm6
