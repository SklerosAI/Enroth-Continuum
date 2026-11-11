#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6MonsterAttack.h"

#include "Library/Random/SequentialRandomEngine.h"

static MonsterInfo archer() {
    MonsterInfo result;
    result.attack1DamageDiceRolls = 2;
    result.attack1DamageDiceSides = 6;
    result.attack1DamageBonus = 2;
    result.attack2DamageDiceRolls = 3;
    result.attack2DamageDiceSides = 6;
    result.attack2DamageBonus = 9;
    return result;
}

UNIT_TEST(MM6MonsterAttack, SecondAttackAddsTheFirstAttackBonus) {
    // The second monster attack used to add its own bonus where the original adds the first attack's.
    SequentialRandomEngine rng;

    rng.seed(0);
    EXPECT_EQ(mm6::monsterAttackDamage(archer(), ABILITY_ATTACK1, rng), 2 + 3 + 2);
    rng.seed(0);
    EXPECT_EQ(mm6::monsterAttackDamage(archer(), ABILITY_ATTACK2, rng), 2 + 3 + 4 + 2);
    rng.seed(0);
    EXPECT_EQ(mm6::monsterAttackDamage(archer(), ABILITY_SPECIAL, rng), 0);
    EXPECT_EQ(rng.state(), 0);
}
