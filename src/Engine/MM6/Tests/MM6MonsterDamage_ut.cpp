#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6MonsterDamage.h"
#include "Engine/MM6/MM6MonsterSpecialAttack.h"
#include "Engine/MM6/MM6MonsterProjectile.h"
#include "Engine/MM6/MM6MonsterSpells.h"
#include "Engine/MM6/MM6MonsterSpellEffect.h"
#include "Engine/Graphics/Sprites.h"
#include "Engine/Snapshots/TableSerialization.h"
#include "Engine/Tables/ObjectTable.h"

#include "Utility/Memory/Blob.h"

UNIT_TEST(MM6MonsterDamage, DecodesAllActiveLabelsWithoutCollision) {
    struct Example {
        std::string_view text;
        mm6::MonsterDamageLabel label;
        DamageType storageType;
    };
    constexpr std::array examples = {
        Example{"Phys", mm6::MonsterDamageLabel::Physical, DAMAGE_PHYSICAL},
        Example{"Fire", mm6::MonsterDamageLabel::Fire, DAMAGE_FIRE},
        Example{"Elec", mm6::MonsterDamageLabel::Electricity, DAMAGE_AIR},
        Example{"Cold", mm6::MonsterDamageLabel::Cold, DAMAGE_WATER},
        Example{"Pois", mm6::MonsterDamageLabel::Poison, DAMAGE_EARTH},
        Example{"Magic", mm6::MonsterDamageLabel::Magic, DAMAGE_MAGIC},
        Example{"Ener", mm6::MonsterDamageLabel::Energy, DAMAGE_ENERGY},
    };
    for (const Example &example : examples) {
        EXPECT_EQ(mm6::parseMonsterDamageLabel(example.text), example.label);
        EXPECT_EQ(mm6::damageTypeForMonsterAttack(example.text, true), example.storageType);
    }
    EXPECT_EQ(mm6::parseMonsterDamageLabel("eLeC"), mm6::MonsterDamageLabel::Electricity);
    EXPECT_NE(mm6::damageTypeForMonsterAttack("Ener", true), DAMAGE_EARTH);
}

UNIT_TEST(MM6MonsterDamage, RejectsUnknownActiveAndIgnoresZeroChancePlaceholder) {
    EXPECT_THROW(mm6::damageTypeForMonsterAttack("0", true), std::runtime_error);
    EXPECT_THROW(mm6::damageTypeForMonsterAttack("Unknown", true), std::runtime_error);
    EXPECT_EQ(mm6::damageTypeForMonsterAttack("0", false), DAMAGE_PHYSICAL);
}

UNIT_TEST(MM6MonsterDamage, GatesUnverifiedEffectsByTitle) {
    for (DamageType type : {DAMAGE_PHYSICAL, DAMAGE_FIRE})
        EXPECT_NO_THROW(mm6::requireVerifiedMonsterDamage(GameTitle::MM6, type));
    for (DamageType type : {DAMAGE_AIR, DAMAGE_WATER, DAMAGE_EARTH, DAMAGE_MAGIC, DAMAGE_ENERGY}) {
        EXPECT_THROW(mm6::requireVerifiedMonsterDamage(GameTitle::MM6, type), std::runtime_error);
        EXPECT_NO_THROW(mm6::requireVerifiedMonsterDamage(GameTitle::MM7, type));
    }
    EXPECT_THROW(mm6::requireVerifiedMonsterDamage(GameTitle::MM6, DAMAGE_MIND), std::runtime_error);
}

UNIT_TEST(MM6MonsterDamage, GatesCharacterChannelsSeparatelyFromMonsterChannels) {
    // Characters take all seven MM6 channels, monsters don't have a verified rule for most of them yet.
    for (DamageType type : {DAMAGE_PHYSICAL, DAMAGE_MAGIC, DAMAGE_FIRE, DAMAGE_AIR, DAMAGE_WATER, DAMAGE_EARTH, DAMAGE_ENERGY})
        EXPECT_NO_THROW(mm6::requireVerifiedCharacterDamage(GameTitle::MM6, type));
    for (DamageType type : {DAMAGE_SPIRIT, DAMAGE_MIND, DAMAGE_BODY, DAMAGE_LIGHT, DAMAGE_DARK}) {
        EXPECT_THROW(mm6::requireVerifiedCharacterDamage(GameTitle::MM6, type), std::runtime_error);
        EXPECT_NO_THROW(mm6::requireVerifiedCharacterDamage(GameTitle::MM7, type));
    }
    for (DamageType type : {DAMAGE_AIR, DAMAGE_WATER, DAMAGE_EARTH, DAMAGE_MAGIC, DAMAGE_ENERGY})
        EXPECT_THROW(mm6::requireVerifiedMonsterDamage(GameTitle::MM6, type), std::runtime_error);
}

UNIT_TEST(MM6MonsterDamage, EventKindsFollowTheMonsterLabelNumbering) {
    // MM6 event damage kinds used to be read in MM7's numbering, so physical traps rolled fire resistance and cold ones none.
    EXPECT_EQ(mm6::damageTypeForEventKind(0), mm6::damageTypeForMonsterAttack("Phys", true));
    EXPECT_EQ(mm6::damageTypeForEventKind(1), mm6::damageTypeForMonsterAttack("Magic", true));
    EXPECT_EQ(mm6::damageTypeForEventKind(2), mm6::damageTypeForMonsterAttack("Fire", true));
    EXPECT_EQ(mm6::damageTypeForEventKind(3), mm6::damageTypeForMonsterAttack("Elec", true));
    EXPECT_EQ(mm6::damageTypeForEventKind(4), mm6::damageTypeForMonsterAttack("Cold", true));
    EXPECT_EQ(mm6::damageTypeForEventKind(5), mm6::damageTypeForMonsterAttack("Pois", true));
    EXPECT_EQ(mm6::damageTypeForEventKind(6), mm6::damageTypeForMonsterAttack("Ener", true));
    EXPECT_EQ(mm6::damageTypeForEventKind(0), DAMAGE_PHYSICAL);
    EXPECT_EQ(mm6::damageTypeForEventKind(4), DAMAGE_WATER);
    EXPECT_EQ(mm6::damageTypeForEventKind(7), DAMAGE_PHYSICAL);
    EXPECT_EQ(mm6::damageTypeForEventKind(255), DAMAGE_PHYSICAL);
}

UNIT_TEST(MM6MonsterDamage, ReadsSixExplicitMonsterResistanceColumns) {
    MonsterInfo monster;
    for (mm6::MonsterDamageLabel label : {mm6::MonsterDamageLabel::Physical,
                                         mm6::MonsterDamageLabel::Fire})
        EXPECT_THROW(mm6::monsterResistanceForAttack(monster, label), std::runtime_error);

    monster.mm6Resistances = mm6::MonsterResistances{
        .fire = 11, .electricity = 22, .cold = 33,
        .poison = 44, .physical = 55, .magic = 66,
    };
    EXPECT_EQ(mm6::monsterResistanceForAttack(monster, mm6::MonsterDamageLabel::Fire), 11);
    EXPECT_EQ(mm6::monsterResistanceForAttack(monster, mm6::MonsterDamageLabel::Electricity), 22);
    EXPECT_EQ(mm6::monsterResistanceForAttack(monster, mm6::MonsterDamageLabel::Cold), 33);
    EXPECT_EQ(mm6::monsterResistanceForAttack(monster, mm6::MonsterDamageLabel::Poison), 44);
    EXPECT_EQ(mm6::monsterResistanceForAttack(monster, mm6::MonsterDamageLabel::Physical), 55);
    EXPECT_EQ(mm6::monsterResistanceForAttack(monster, mm6::MonsterDamageLabel::Magic), 66);
    EXPECT_THROW(mm6::monsterResistanceForAttack(monster, mm6::MonsterDamageLabel::Energy), std::runtime_error);
}

UNIT_TEST(MM6MonsterDamage, RetainsSixF3MatchedMonsterResistanceColumns) {
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!research)
        GTEST_SKIP() << "Set ignored MM6 F3 research directory";
    const std::filesystem::path path = std::filesystem::path(research) / "icons/monsters.txt";
    std::ifstream stream(path, std::ios::binary);
    ASSERT_TRUE(stream.is_open());
    std::string data(std::istreambuf_iterator<char>{stream}, {});
    MonsterStats stats;
    stats.Initialize(Blob::fromString(std::move(data)), GameTitle::MM6);

    std::array<int, 6> immuneCounts{};
    int rows = 0;
    for (int rawId = 1; rawId <= 173; ++rawId) {
        const MonsterId id = static_cast<MonsterId>(rawId);
        const MonsterInfo &info = stats.infos[id];
        ASSERT_TRUE(info.mm6Resistances.has_value()) << "monster ID " << rawId;
        const mm6::MonsterResistances &res = *info.mm6Resistances;
        EXPECT_EQ(res.fire, info.resFire);
        EXPECT_EQ(res.electricity, info.resAir);
        EXPECT_EQ(res.cold, info.resWater);
        EXPECT_EQ(res.poison, info.resEarth);
        EXPECT_EQ(res.physical, info.resPhysical);
        EXPECT_EQ(res.magic, info.resMind);
        EXPECT_EQ(res.magic, info.resDark);
        const std::array<uint8_t, 6> columns = {
            res.fire, res.electricity, res.cold, res.poison, res.physical, res.magic,
        };
        for (size_t index = 0; index < columns.size(); ++index)
            immuneCounts[index] += columns[index] == 200;
        ++rows;
    }
    EXPECT_EQ(rows, 173);
    EXPECT_EQ(immuneCounts, (std::array<int, 6>{18, 14, 13, 45, 5, 54}));
}

UNIT_TEST(MM6MonsterSpecialAttack, ReadsTheFirstNameTheCellContains) {
    // The land dragons' Pois2 and Pois3 used to poison, the original finds none of its names in them.
    EXPECT_EQ(mm6::parseMonsterSpecialAttackName("Affraid"), SPECIAL_ATTACK_FEAR);
    EXPECT_EQ(mm6::parseMonsterSpecialAttackName("Pois2"), SPECIAL_ATTACK_NONE);
    EXPECT_EQ(mm6::parseMonsterSpecialAttackName("Pois3"), SPECIAL_ATTACK_NONE);
    EXPECT_EQ(mm6::parseMonsterSpecialAttackName("Poison2"), SPECIAL_ATTACK_POISON_MEDIUM);
    EXPECT_EQ(mm6::parseMonsterSpecialAttackName("BrkArmor "), SPECIAL_ATTACK_BREAK_ARMOR);
    EXPECT_EQ(mm6::parseMonsterSpecialAttackName("Brkweapon"), SPECIAL_ATTACK_BREAK_WEAPON);
    EXPECT_EQ(mm6::parseMonsterSpecialAttackName("Age"), SPECIAL_ATTACK_AGING);
    EXPECT_EQ(mm6::parseMonsterSpecialAttackName("unsupported"), SPECIAL_ATTACK_NONE);
}

UNIT_TEST(MM6MonsterSpecialAttack, CoversAllF3MatchedActiveMonsterRows) {
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!research)
        GTEST_SKIP() << "Set ignored MM6 F3 research directory";
    const std::filesystem::path path = std::filesystem::path(research) / "icons/monsters.txt";
    std::ifstream stream(path, std::ios::binary);
    ASSERT_TRUE(stream.is_open());
    std::string data(std::istreambuf_iterator<char>{stream}, {});
    MonsterStats stats;
    stats.Initialize(Blob::fromString(std::move(data)), GameTitle::MM6);
    int active = 0;
    int fear = 0;
    int mediumPoison = 0;
    int severePoison = 0;
    for (int rawId = 1; rawId <= 173; ++rawId) {
        const MonsterSpecialAttack type = stats.infos[static_cast<MonsterId>(rawId)].specialAttackType;
        active += type != SPECIAL_ATTACK_NONE;
        fear += type == SPECIAL_ATTACK_FEAR;
        mediumPoison += type == SPECIAL_ATTACK_POISON_MEDIUM;
        severePoison += type == SPECIAL_ATTACK_POISON_SEVERE;
    }
    EXPECT_EQ(active, 90);
    EXPECT_EQ(fear, 8);
    EXPECT_EQ(mediumPoison, 3);
    EXPECT_EQ(severePoison, 2);
}

UNIT_TEST(MM6MonsterSpells, ResolvesMM6IdsAndRejectsUnknownNames) {
    EXPECT_EQ(std::to_underlying(mm6::monsterSpellId("Fire Bolt")), 4);
    EXPECT_EQ(std::to_underlying(mm6::monsterSpellId("Ice Bolt")), 28);
    EXPECT_EQ(std::to_underlying(mm6::monsterSpellId("Mind Blast")), 58);
    EXPECT_EQ(std::to_underlying(mm6::monsterSpellId("Dispell Magic")), 80);
    EXPECT_EQ(std::to_underlying(mm6::monsterSpellId("Psychic Shockt")), 65);
    EXPECT_THROW(mm6::monsterSpellId("unsupported"), std::runtime_error);
}

UNIT_TEST(MM6MonsterSpells, SpellsUseOriginalMonsterDiceAndOwnElement) {
    // Monster casts outside the first map's four spells used to refuse. They roll the original monster table, not the
    // player spell formula.
    for (int id : {2, 4, 6, 9, 11, 18, 26, 28, 37, 65, 70, 76}) {
        const mm6::MonsterSpellDice dice = mm6::monsterSpellDice(static_cast<SpellId>(id), 7);
        EXPECT_EQ(dice.rolls, 1) << id;
        EXPECT_EQ(dice.sides, 2) << id;
    }
    for (int id : {24, 58, 95}) {
        const mm6::MonsterSpellDice dice = mm6::monsterSpellDice(static_cast<SpellId>(id), 3);
        EXPECT_EQ(dice.rolls, 3) << id;
        EXPECT_EQ(dice.sides, 2) << id;
    }
    EXPECT_TRUE(mm6::isVerifiedMonsterSpell(static_cast<SpellId>(91)));
    EXPECT_FALSE(mm6::isVerifiedMonsterSpell(static_cast<SpellId>(86)));
    EXPECT_FALSE(mm6::isVerifiedMonsterSpell(static_cast<SpellId>(90)));

    EXPECT_EQ(mm6::monsterSpellDamageType(mm6::SpellDamageTag::Physical), DAMAGE_PHYSICAL);
    EXPECT_EQ(mm6::monsterSpellDamageType(mm6::SpellDamageTag::Fire), DAMAGE_FIRE);
    EXPECT_EQ(mm6::monsterSpellDamageType(mm6::SpellDamageTag::Electricity), DAMAGE_AIR);
    EXPECT_EQ(mm6::monsterSpellDamageType(mm6::SpellDamageTag::Cold), DAMAGE_WATER);
    EXPECT_EQ(mm6::monsterSpellDamageType(mm6::SpellDamageTag::Poison), DAMAGE_EARTH);
    EXPECT_EQ(mm6::monsterSpellDamageType(mm6::SpellDamageTag::Magic), DAMAGE_MAGIC);
    EXPECT_THROW(mm6::monsterSpellDamageType(mm6::SpellDamageTag::None), std::runtime_error);
}

UNIT_TEST(MM6MonsterSpells, SpellsLaunchTheirOriginalObjects) {
    // Monster casts outside the first map's four spells used to refuse. Expert Meteor Shower drops the most meteors.
    EXPECT_EQ(mm6::monsterSpellObject(static_cast<SpellId>(6)), static_cast<SpriteId>(1050));
    EXPECT_EQ(mm6::monsterSpellObject(static_cast<SpellId>(37)), static_cast<SpriteId>(4030));
    EXPECT_EQ(mm6::monsterSpellObject(static_cast<SpellId>(95)), static_cast<SpriteId>(9060));
    EXPECT_THROW(mm6::monsterSpellObject(static_cast<SpellId>(80)), std::runtime_error);
    EXPECT_EQ(mm6::monsterMeteorCount(CombinedSkillValue(10, MASTERY_NOVICE)), 8);
    EXPECT_EQ(mm6::monsterMeteorCount(CombinedSkillValue(10, MASTERY_EXPERT)), 16);
    EXPECT_EQ(mm6::monsterMeteorCount(CombinedSkillValue(10, MASTERY_MASTER)), 12);
    EXPECT_EQ(mm6::monsterSkillByte(CombinedSkillValue(14, MASTERY_NOVICE)), 14);
    EXPECT_EQ(mm6::monsterSkillByte(CombinedSkillValue(10, MASTERY_MASTER)), 0x8A);
}

UNIT_TEST(MM6MonsterSpells, RetainsAllF3MatchedActiveMonsterCasts) {
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!research)
        GTEST_SKIP() << "Set ignored MM6 F3 research directory";
    const std::filesystem::path path = std::filesystem::path(research) / "icons/monsters.txt";
    std::ifstream stream(path, std::ios::binary);
    ASSERT_TRUE(stream.is_open());
    std::string data(std::istreambuf_iterator<char>{stream}, {});
    MonsterStats stats;
    stats.Initialize(Blob::fromString(std::move(data)), GameTitle::MM6);
    int active = 0;
    for (int rawId = 1; rawId <= 173; ++rawId) {
        const MonsterInfo &info = stats.infos[static_cast<MonsterId>(rawId)];
        active += info.spell1UseChance > 0;
        if (info.spell1UseChance > 0)
            EXPECT_NE(info.spell1Id, SPELL_NONE) << "monster ID " << rawId;
    }
    EXPECT_EQ(active, 55);
}

UNIT_TEST(MM6MonsterProjectile, EachAttackColumnReadsItsOwnNames) {
    // Second-attack Pois and Dagger cells used to get missiles that the original never launches.
    EXPECT_EQ(mm6::monsterProjectileType("Arrow", mm6::MonsterAttackColumn::Attack1), MONSTER_PROJECTILE_MM6_ARROW);
    EXPECT_EQ(mm6::monsterProjectileType("ArrowF", mm6::MonsterAttackColumn::Attack1), MONSTER_PROJECTILE_MM6_FLAMING_ARROW);
    EXPECT_EQ(mm6::monsterProjectileType("FireAr", mm6::MonsterAttackColumn::Attack1), MONSTER_PROJECTILE_NONE);
    EXPECT_EQ(mm6::monsterProjectileType("Pois", mm6::MonsterAttackColumn::Attack1), MONSTER_PROJECTILE_MM6_POISON);
    EXPECT_EQ(mm6::monsterProjectileType("FireAr", mm6::MonsterAttackColumn::Attack2), MONSTER_PROJECTILE_MM6_FLAMING_ARROW);
    EXPECT_EQ(mm6::monsterProjectileType("Pois", mm6::MonsterAttackColumn::Attack2), MONSTER_PROJECTILE_NONE);
    EXPECT_EQ(mm6::monsterProjectileType("Poison", mm6::MonsterAttackColumn::Attack2), MONSTER_PROJECTILE_MM6_POISON);
    EXPECT_EQ(mm6::monsterProjectileType("Dagger", mm6::MonsterAttackColumn::Attack2), MONSTER_PROJECTILE_NONE);
    EXPECT_EQ(mm6::monsterProjectileType("magic", mm6::MonsterAttackColumn::Attack2), MONSTER_PROJECTILE_MM6_MAGIC);
    EXPECT_EQ(mm6::monsterProjectileType("0", mm6::MonsterAttackColumn::Attack1), MONSTER_PROJECTILE_NONE);
}

UNIT_TEST(MM6MonsterProjectile, MissilesLaunchTheirOriginalObjects) {
    // MM6 monster missiles used to refuse to launch. Robots shoot Energy as their own laser, the rest as the Energy missile.
    EXPECT_EQ(mm6::monsterProjectileObjectId(MONSTER_PROJECTILE_MM6_ARROW, static_cast<MonsterId>(1)), static_cast<SpriteId>(500));
    EXPECT_EQ(mm6::monsterProjectileObjectId(MONSTER_PROJECTILE_MM6_ROCK, static_cast<MonsterId>(52)), static_cast<SpriteId>(580));
    EXPECT_EQ(mm6::monsterProjectileObjectId(MONSTER_PROJECTILE_MM6_ENERGY, static_cast<MonsterId>(147)), static_cast<SpriteId>(560));
    for (int robot : {148, 149, 150})
        EXPECT_EQ(mm6::monsterProjectileObjectId(MONSTER_PROJECTILE_MM6_ENERGY, static_cast<MonsterId>(robot)), static_cast<SpriteId>(590));
    EXPECT_EQ(mm6::monsterProjectileObjectId(MONSTER_PROJECTILE_MM6_ENERGY, static_cast<MonsterId>(173)), static_cast<SpriteId>(560));
    EXPECT_THROW(mm6::monsterProjectileObjectId(MONSTER_PROJECTILE_NONE, static_cast<MonsterId>(1)), std::runtime_error);
}

UNIT_TEST(MM6MonsterProjectile, ObjectsLandAsTheOriginalCollisionDecides) {
    // Flame Arrow used to explode like a bolt where the original lets it vanish like an arrow.
    for (int object : {500, 510, 1010})
        EXPECT_EQ(mm6::monsterObjectImpact(static_cast<SpriteId>(object)), mm6::MonsterObjectImpact::Arrow) << object;
    for (int object : {520, 530, 540, 550, 560, 570, 580, 590, 1030, 2060, 3010})
        EXPECT_EQ(mm6::monsterObjectImpact(static_cast<SpriteId>(object)), mm6::MonsterObjectImpact::Missile) << object;
    EXPECT_EQ(mm6::monsterObjectImpact(static_cast<SpriteId>(1050)), mm6::MonsterObjectImpact::Explosion);
    EXPECT_EQ(mm6::monsterObjectImpact(static_cast<SpriteId>(1080)), mm6::MonsterObjectImpact::Meteor);
    EXPECT_EQ(mm6::monsterObjectImpact(static_cast<SpriteId>(8080)), mm6::MonsterObjectImpact::Unverified);
    EXPECT_EQ(mm6::monsterObjectExplosion(static_cast<SpriteId>(520)), static_cast<SpriteId>(521));
    EXPECT_EQ(mm6::monsterObjectExplosion(static_cast<SpriteId>(590)), static_cast<SpriteId>(561));
}

UNIT_TEST(MM6MonsterProjectile, RetainsAllF3MatchedRangedAttackCells) {
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!research)
        GTEST_SKIP() << "Set ignored MM6 F3 research directory";
    const std::filesystem::path path = std::filesystem::path(research) / "icons/monsters.txt";
    std::ifstream stream(path, std::ios::binary);
    ASSERT_TRUE(stream.is_open());
    std::string data(std::istreambuf_iterator<char>{stream}, {});
    MonsterStats stats;
    stats.Initialize(Blob::fromString(std::move(data)), GameTitle::MM6);

    std::array<int, 9> counts{};
    for (int rawId = 1; rawId <= 173; ++rawId) {
        const MonsterInfo &info = stats.infos[static_cast<MonsterId>(rawId)];
        for (MonsterProjectile type : {info.attack1MissileType, info.attack2MissileType}) {
            if (type == MONSTER_PROJECTILE_NONE)
                continue;
            const int index = std::to_underlying(type) - 100;
            ASSERT_GE(index, 0);
            ASSERT_LT(index, static_cast<int>(counts.size()));
            ++counts[index];
        }
    }
    EXPECT_EQ(counts, (std::array<int, 9>{6, 18, 17, 10, 12, 3, 4, 3, 3}));
}

UNIT_TEST(MM6MonsterProjectile, NineLabelsResolveToRetainedMM6ObjectAndSpriteRows) {
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!research)
        GTEST_SKIP() << "Set ignored MM6 F3 research directory";
    const std::filesystem::path path = std::filesystem::path(research) / "icons/dobjlist.bin";
    std::ifstream stream(path, std::ios::binary);
    ASSERT_TRUE(stream.is_open());
    std::string data(std::istreambuf_iterator<char>{stream}, {});
    ObjectTable objects;
    deserialize(Blob::fromString(std::move(data)), &objects, GameTitle::MM6);
    ASSERT_EQ(objects.pObjects.size(), 232);

    std::ifstream frameStream(std::filesystem::path(research) / "icons/dsft.bin", std::ios::binary);
    ASSERT_TRUE(frameStream.is_open());
    std::string frameData(std::istreambuf_iterator<char>{frameStream}, {});
    SpriteFrameTable frames;
    deserialize(Blob::fromString(std::move(frameData)), &frames, GameTitle::MM6);
    ASSERT_EQ(frames.pSpriteSFrames.size(), 6455);

    constexpr std::array labels = {
        MONSTER_PROJECTILE_MM6_ARROW, MONSTER_PROJECTILE_MM6_FLAMING_ARROW,
        MONSTER_PROJECTILE_MM6_FIRE, MONSTER_PROJECTILE_MM6_ELECTRICITY,
        MONSTER_PROJECTILE_MM6_COLD, MONSTER_PROJECTILE_MM6_POISON,
        MONSTER_PROJECTILE_MM6_ENERGY, MONSTER_PROJECTILE_MM6_MAGIC,
        MONSTER_PROJECTILE_MM6_ROCK,
    };
    constexpr std::array names = {"msl01", "msl02", "msl03", "msl04", "msl05",
                                  "msl06", "msl07", "msl08", "msl09"};
    for (size_t index = 0; index < labels.size(); ++index) {
        MonsterProjectile label = labels[index];
        const SpriteId objectId = mm6::monsterProjectileObjectId(label, static_cast<MonsterId>(1));
        const unsigned row = objects.ObjectIDByItemID(objectId);
        ASSERT_GT(row, 0);
        ASSERT_LT(row, objects.pObjects.size());
        EXPECT_EQ(objects.pObjects[row].uObjectID, objectId);
        const unsigned frameId = objects.pObjects[row].uSpriteID;
        ASSERT_GT(frameId, 0);
        ASSERT_LT(frameId, frames.pSpriteSFrames.size());
        EXPECT_EQ(frames.pSpriteSFrames[frameId].spriteName, names[index]);
        EXPECT_FALSE(frames.pSpriteSFrames[frameId].textureName.empty());
    }
    constexpr std::array impactLabels = {
        MONSTER_PROJECTILE_MM6_FIRE, MONSTER_PROJECTILE_MM6_ELECTRICITY,
        MONSTER_PROJECTILE_MM6_COLD, MONSTER_PROJECTILE_MM6_POISON,
        MONSTER_PROJECTILE_MM6_ENERGY, MONSTER_PROJECTILE_MM6_MAGIC,
        MONSTER_PROJECTILE_MM6_ROCK,
    };
    constexpr std::array impactNames = {
        "mslx03", "mslx04", "mslx05", "mslx06", "mslx07", "mslx08", "mslx09",
    };
    for (size_t index = 0; index < impactLabels.size(); ++index) {
        const SpriteId objectId = mm6::monsterObjectExplosion(mm6::monsterProjectileObjectId(impactLabels[index], static_cast<MonsterId>(1)));
        const unsigned row = objects.ObjectIDByItemID(objectId);
        ASSERT_GT(row, 0);
        ASSERT_LT(row, objects.pObjects.size());
        EXPECT_EQ(objects.pObjects[row].uObjectID, objectId);
        EXPECT_EQ(objects.pObjects[row].uSpeed, 0);
        const unsigned frameId = objects.pObjects[row].uSpriteID;
        ASSERT_LT(frameId, frames.pSpriteSFrames.size());
        EXPECT_EQ(frames.pSpriteSFrames[frameId].spriteName, impactNames[index]);
        EXPECT_FALSE(frames.pSpriteSFrames[frameId].textureName.empty());
    }
}
