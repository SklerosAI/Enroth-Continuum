#pragma once

#include <array>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "Engine/Objects/MonsterEnums.h"
#include "Engine/Objects/SpriteEnums.h"
#include "Utility/String/Ascii.h"

namespace mm6 {

/**
 * Attack column of a monster in MM6 `monsters.txt`.
 */
enum class MonsterAttackColumn {
    Attack1,
    Attack2
};

/**
 * @param label                         Missile cell of a monster attack in MM6 `monsters.txt`.
 * @param column                        Which attack the cell belongs to.
 * @return                              Missile the original reads from the cell. Each column compares against its own
 *                                      nine names, and a cell matching none of them leaves the attack without a
 *                                      missile.
 */
inline MonsterProjectile monsterProjectileType(std::string_view label, MonsterAttackColumn column) {
    constexpr std::array<std::pair<std::string_view, MonsterProjectile>, 9> attack1 = {{
        {"Arrow", MONSTER_PROJECTILE_MM6_ARROW},
        {"ArrowF", MONSTER_PROJECTILE_MM6_FLAMING_ARROW},
        {"Fire", MONSTER_PROJECTILE_MM6_FIRE},
        {"Elec", MONSTER_PROJECTILE_MM6_ELECTRICITY},
        {"Cold", MONSTER_PROJECTILE_MM6_COLD},
        {"Pois", MONSTER_PROJECTILE_MM6_POISON},
        {"Ener", MONSTER_PROJECTILE_MM6_ENERGY},
        {"Magic", MONSTER_PROJECTILE_MM6_MAGIC},
        {"Rock", MONSTER_PROJECTILE_MM6_ROCK},
    }};
    constexpr std::array<std::pair<std::string_view, MonsterProjectile>, 9> attack2 = {{
        {"Arrow", MONSTER_PROJECTILE_MM6_ARROW},
        {"FireAr", MONSTER_PROJECTILE_MM6_FLAMING_ARROW},
        {"Fire", MONSTER_PROJECTILE_MM6_FIRE},
        {"Elec", MONSTER_PROJECTILE_MM6_ELECTRICITY},
        {"Cold", MONSTER_PROJECTILE_MM6_COLD},
        {"Poison", MONSTER_PROJECTILE_MM6_POISON},
        {"Ener", MONSTER_PROJECTILE_MM6_ENERGY},
        {"Magic", MONSTER_PROJECTILE_MM6_MAGIC},
        {"Rock", MONSTER_PROJECTILE_MM6_ROCK},
    }};
    for (const auto &[name, type] : column == MonsterAttackColumn::Attack1 ? attack1 : attack2)
        if (ascii::noCaseEquals(label, name))
            return type;
    return MONSTER_PROJECTILE_NONE;
}

/**
 * @param value                         Missile byte of a monster attack in an original MM6 actor record.
 * @return                              Missile it stands for, numbered the way the `monsters.txt` loader numbers them.
 */
inline MonsterProjectile monsterProjectileForNativeByte(int value) {
    constexpr std::array<MonsterProjectile, 10> types = {
        MONSTER_PROJECTILE_NONE, MONSTER_PROJECTILE_MM6_ARROW, MONSTER_PROJECTILE_MM6_FLAMING_ARROW,
        MONSTER_PROJECTILE_MM6_FIRE, MONSTER_PROJECTILE_MM6_ELECTRICITY, MONSTER_PROJECTILE_MM6_COLD,
        MONSTER_PROJECTILE_MM6_POISON, MONSTER_PROJECTILE_MM6_ENERGY, MONSTER_PROJECTILE_MM6_MAGIC,
        MONSTER_PROJECTILE_MM6_ROCK,
    };
    return value >= 0 && value < static_cast<int>(types.size()) ? types[value] : MONSTER_PROJECTILE_NONE;
}

/**
 * @param type                          MM6 missile of a monster attack.
 * @param monster                       Shooting monster.
 * @return                              Object the original launches for it. The robots shoot Energy as their own laser.
 */
inline SpriteId monsterProjectileObjectId(MonsterProjectile type, MonsterId monster) {
    switch (type) {
        case MONSTER_PROJECTILE_MM6_ARROW: return static_cast<SpriteId>(500);
        case MONSTER_PROJECTILE_MM6_FLAMING_ARROW: return static_cast<SpriteId>(510);
        case MONSTER_PROJECTILE_MM6_FIRE: return static_cast<SpriteId>(520);
        case MONSTER_PROJECTILE_MM6_ELECTRICITY: return static_cast<SpriteId>(530);
        case MONSTER_PROJECTILE_MM6_COLD: return static_cast<SpriteId>(540);
        case MONSTER_PROJECTILE_MM6_POISON: return static_cast<SpriteId>(550);
        case MONSTER_PROJECTILE_MM6_ENERGY: return static_cast<SpriteId>((std::to_underlying(monster) - 1) / 3 == 49 ? 590 : 560);
        case MONSTER_PROJECTILE_MM6_MAGIC: return static_cast<SpriteId>(570);
        case MONSTER_PROJECTILE_MM6_ROCK: return static_cast<SpriteId>(580);
        default: break;
    }
    throw std::runtime_error("Not an MM6 monster projectile");
}

/**
 * What an object a monster launched does when it hits something, in the original object collision routine.
 */
enum class MonsterObjectImpact {
    Arrow, ///< Damages what it hit and disappears.
    Missile, ///< Damages what it hit, stops and turns into its explosion object.
    Explosion, ///< Stops, turns into its explosion object and damages everyone around it.
    Meteor, ///< Flies through monsters and explodes on anything else.
    FingerOfDeath, ///< Can strike what it hit dead, then stops and turns into its explosion object.
    Unverified,
};

/**
 * @param object                        Object a monster launched.
 * @return                              What it does on impact.
 */
inline MonsterObjectImpact monsterObjectImpact(SpriteId object) {
    switch (std::to_underlying(object)) {
        case 500:
        case 510:
        case 1010:
            return MonsterObjectImpact::Arrow;
        case 520: case 530: case 540: case 550: case 560: case 570: case 580: case 590:
        case 1030: case 1100: case 2060: case 3010: case 3030: case 3050: case 4030:
        case 6020: case 6090: case 7030: case 7090:
            return MonsterObjectImpact::Missile;
        case 1050:
            return MonsterObjectImpact::Explosion;
        case 1080:
            return MonsterObjectImpact::Meteor;
        case 9060:
            return MonsterObjectImpact::FingerOfDeath;
        default:
            return MonsterObjectImpact::Unverified;
    }
}

/**
 * @param object                        Object a monster launched that explodes on impact.
 * @return                              Its explosion object. The robot laser explodes as Energy.
 */
inline SpriteId monsterObjectExplosion(SpriteId object) {
    return static_cast<SpriteId>(std::to_underlying(object) == 590 ? 561 : std::to_underlying(object) + 1);
}

}  // namespace mm6
