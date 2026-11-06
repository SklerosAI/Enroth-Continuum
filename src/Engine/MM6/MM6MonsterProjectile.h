#pragma once

#include <array>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "Engine/GameTitle.h"
#include "Engine/Objects/MonsterEnums.h"
#include "Engine/Objects/SpriteEnums.h"
#include "Utility/String/Ascii.h"

namespace mm6 {

// Source types are deliberately separate from inherited MM7 projectile IDs.
// Every nonempty retained MM6 cell has an explicit label.
inline MonsterProjectile monsterProjectileType(std::string_view label, bool active) {
    if (!active || label.empty() || label == "0")
        return MONSTER_PROJECTILE_NONE;
    constexpr std::array<std::pair<std::string_view, MonsterProjectile>, 10> names = {{
        {"Arrow", MONSTER_PROJECTILE_MM6_ARROW},
        {"Fire", MONSTER_PROJECTILE_MM6_FIRE},
        {"Elec", MONSTER_PROJECTILE_MM6_ELECTRICITY},
        {"Cold", MONSTER_PROJECTILE_MM6_COLD},
        {"Ener", MONSTER_PROJECTILE_MM6_ENERGY},
        {"Pois", MONSTER_PROJECTILE_MM6_POISON},
        {"Magic", MONSTER_PROJECTILE_MM6_MAGIC},
        {"FireAr", MONSTER_PROJECTILE_MM6_FLAMING_ARROW},
        {"Rock", MONSTER_PROJECTILE_MM6_ROCK},
        {"Dagger", MONSTER_PROJECTILE_MM6_DAGGER},
    }};
    for (const auto &[name, type] : names)
        if (ascii::noCaseEquals(label, name))
            return type;
    throw std::runtime_error("Unknown active MM6 monster projectile label");
}

inline void requireVerifiedMonsterProjectile(GameTitle title, MonsterProjectile type) {
    if (title == GameTitle::MM6 && type != MONSTER_PROJECTILE_NONE)
        throw std::runtime_error("MM6 monster projectile launch is unverified");
}

// Nine source labels join directly to retained MM6 dobjlist.bin object IDs.
// Dagger has no proved projectile object; the ordinary dagger item row is not
// enough to infer its launch and impact behavior.
inline SpriteId monsterProjectileObjectId(MonsterProjectile type) {
    switch (type) {
        case MONSTER_PROJECTILE_MM6_ARROW: return static_cast<SpriteId>(500);
        case MONSTER_PROJECTILE_MM6_FLAMING_ARROW: return static_cast<SpriteId>(510);
        case MONSTER_PROJECTILE_MM6_FIRE: return static_cast<SpriteId>(520);
        case MONSTER_PROJECTILE_MM6_ELECTRICITY: return static_cast<SpriteId>(530);
        case MONSTER_PROJECTILE_MM6_COLD: return static_cast<SpriteId>(540);
        case MONSTER_PROJECTILE_MM6_POISON: return static_cast<SpriteId>(550);
        case MONSTER_PROJECTILE_MM6_ENERGY: return static_cast<SpriteId>(560);
        case MONSTER_PROJECTILE_MM6_MAGIC: return static_cast<SpriteId>(570);
        case MONSTER_PROJECTILE_MM6_ROCK: return static_cast<SpriteId>(580);
        default: break;
    }
    throw std::runtime_error("MM6 projectile has no verified object ID");
}

}  // namespace mm6
