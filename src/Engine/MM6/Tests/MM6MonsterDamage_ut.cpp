#include <array>
#include <stdexcept>
#include <string_view>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6MonsterDamage.h"

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
