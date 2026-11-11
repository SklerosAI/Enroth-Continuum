#include <memory>
#include <stdexcept>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6MonsterAttack.h"
#include "Engine/Objects/Character.h"
#include "Engine/Party.h"

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

UNIT_TEST(MM6MonsterAttack, PoisonAndMagicSpecialAttacksSaveWithRawResistance) {
    // Every MM6 special attack but breaking an item used to refuse. Poison saves with the raw Poison resistance, the
    // deadly ones with the raw Magic resistance, not with a resistance bonus.
    Character character;
    character.sResBodyBase = 7;
    character.sResMagicBase = 11;
    auto party = std::make_unique<Party>();

    for (MonsterSpecialAttack type : {SPECIAL_ATTACK_POISON_WEAK, SPECIAL_ATTACK_POISON_MEDIUM, SPECIAL_ATTACK_POISON_SEVERE})
        EXPECT_EQ(mm6::specialAttackSave(character, type, *party, 20), 27) << std::to_underlying(type);
    for (MonsterSpecialAttack type : {SPECIAL_ATTACK_PARALYZED, SPECIAL_ATTACK_DEAD, SPECIAL_ATTACK_PETRIFIED,
                                      SPECIAL_ATTACK_ERADICATED, SPECIAL_ATTACK_AGING, SPECIAL_ATTACK_MANA_DRAIN})
        EXPECT_EQ(mm6::specialAttackSave(character, type, *party, 20), 11) << std::to_underlying(type);
    EXPECT_THROW(mm6::specialAttackSave(character, SPECIAL_ATTACK_BREAK_ANY, *party, 0), std::runtime_error);
}
