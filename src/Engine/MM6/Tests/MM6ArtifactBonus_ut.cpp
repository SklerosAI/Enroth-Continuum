#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ArtifactBonus.h"

UNIT_TEST(MM6ArtifactBonus, GuinevereBoostsLightAndDarkOnAnyRing) {
    // Guinevere didn't boost Light and Dark spells. No Light or Dark player spell is admitted yet to show it in game.
    const ItemId guinevere = static_cast<ItemId>(412);
    for (ItemSlot ring : {ITEM_SLOT_RING1, ITEM_SLOT_RING6}) {
        EXPECT_TRUE(mm6::boostsSpellSchool(guinevere, ring, SKILL_LIGHT));
        EXPECT_TRUE(mm6::boostsSpellSchool(guinevere, ring, SKILL_DARK));
        EXPECT_FALSE(mm6::boostsSpellSchool(guinevere, ring, SKILL_SPIRIT));
        EXPECT_FALSE(mm6::boostsSpellSchool(guinevere, ring, SKILL_FIRE));
    }
    EXPECT_FALSE(mm6::boostsSpellSchool(static_cast<ItemId>(413), ITEM_SLOT_RING1, SKILL_LIGHT)); // Igraine.
}
