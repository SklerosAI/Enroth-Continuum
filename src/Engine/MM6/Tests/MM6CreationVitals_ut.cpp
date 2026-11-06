#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6CreationVitals.h"

namespace {

struct CharacterWithStartingItemBonus {
    int health = -1;
    int mana = -1;
    int itemHealthBonus = 0;
    int itemManaBonus = 0;

    int GetMaxHealth() const { return 10 + itemHealthBonus; }
    int GetMaxMana() const { return 5 + itemManaBonus; }
};

}  // namespace

UNIT_TEST(MM6CreationVitals, RefillsAfterItemGrantsOnlyForMM6) {
    CharacterWithStartingItemBonus mm6Character;
    mm6::refillCreationVitalsAtStage(GameTitle::MM6,
                                    mm6::CreationVitalStage::BeforeStartingItems, &mm6Character);
    EXPECT_EQ(mm6Character.health, -1);
    EXPECT_EQ(mm6Character.mana, -1);
    mm6Character.itemHealthBonus = 7;
    mm6Character.itemManaBonus = 3;
    mm6::refillCreationVitalsAtStage(GameTitle::MM6,
                                    mm6::CreationVitalStage::AfterStartingItems, &mm6Character);
    EXPECT_EQ(mm6Character.health, 17);
    EXPECT_EQ(mm6Character.mana, 8);

    CharacterWithStartingItemBonus mm7Character;
    mm6::refillCreationVitalsAtStage(GameTitle::MM7,
                                    mm6::CreationVitalStage::BeforeStartingItems, &mm7Character);
    EXPECT_EQ(mm7Character.health, 10);
    EXPECT_EQ(mm7Character.mana, 5);
    mm7Character.itemHealthBonus = 7;
    mm7Character.itemManaBonus = 3;
    mm6::refillCreationVitalsAtStage(GameTitle::MM7,
                                    mm6::CreationVitalStage::AfterStartingItems, &mm7Character);
    EXPECT_EQ(mm7Character.health, 10);
    EXPECT_EQ(mm7Character.mana, 5);
}
