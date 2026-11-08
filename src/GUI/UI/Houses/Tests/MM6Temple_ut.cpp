#include "Testing/Unit/UnitTest.h"

#include "GUI/UI/Houses/MM6Temple.h"

#include "Engine/Objects/Character.h"

UNIT_TEST(MM6Temple, HealPriceUsesConditionStartDay) {
    // MM6 multiplies the heal price by the game day a condition started on, mod 7, plus 1.
    Character character;
    EXPECT_EQ(mm6::templeHealPrice(character, 10), 10);

    character.conditions.set(CONDITION_WEAK, Time::fromDays(3) + Duration::fromHours(5));
    EXPECT_EQ(mm6::templeHealPrice(character, 10), 40);
    character.conditions.set(CONDITION_POISON_WEAK, Time::fromDays(12));
    EXPECT_EQ(mm6::templeHealPrice(character, 10), 60);
    character.conditions.set(CONDITION_UNCONSCIOUS, Time::fromDays(13));
    EXPECT_EQ(mm6::templeHealPrice(character, 10), 70);

    character.conditions.set(CONDITION_DEAD, Time::fromDays(9));
    EXPECT_EQ(mm6::templeHealPrice(character, 10), 150);
    character.conditions.set(CONDITION_PETRIFIED, Time::fromDays(7));
    EXPECT_EQ(mm6::templeHealPrice(character, 10), 50);
    character.conditions.set(CONDITION_ERADICATED, Time::fromDays(15));
    EXPECT_EQ(mm6::templeHealPrice(character, 10), 200);
}

UNIT_TEST(MM6Temple, HealPriceTruncatesAndHasNoCap) {
    // MM6 truncates the heal price, raises it to at least 1 and has no upper limit.
    Character character;
    EXPECT_EQ(mm6::templeHealPrice(character, 0.5f), 1);
    EXPECT_EQ(mm6::templeHealPrice(character, 2.5f), 2);
    character.conditions.set(CONDITION_ERADICATED, Time::fromDays(6));
    EXPECT_EQ(mm6::templeHealPrice(character, 300), 21000);
}
