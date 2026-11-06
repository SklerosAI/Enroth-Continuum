#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6MonsterDamage.h"

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
