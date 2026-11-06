#include <stdexcept>
#include <initializer_list>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6SpellItems.h"

UNIT_TEST(MM6SpellItems, EveryRetainedScrollAndBookMapsToItsSpellId) {
    for (int spell = 1; spell <= 99; ++spell) {
        ItemId scroll = static_cast<ItemId>(199 + spell);
        ItemId book = static_cast<ItemId>(299 + spell);
        EXPECT_TRUE(mm6::isSpellScrollId(scroll));
        EXPECT_TRUE(mm6::isSpellbookId(book));
        EXPECT_EQ(mm6::spellForScroll(scroll), static_cast<SpellId>(spell));
        EXPECT_EQ(mm6::spellForSpellbook(book), static_cast<SpellId>(spell));
    }
}

UNIT_TEST(MM6SpellItems, RejectsEdgesAndWrongItemKinds) {
    for (int id : {0, 199, 299, 399, 400, 498}) {
        ItemId item = static_cast<ItemId>(id);
        if (!mm6::isSpellScrollId(item))
            EXPECT_THROW((void) mm6::spellForScroll(item), std::out_of_range);
        if (!mm6::isSpellbookId(item))
            EXPECT_THROW((void) mm6::spellForSpellbook(item), std::out_of_range);
    }
    EXPECT_THROW((void) mm6::spellForSpellbook(static_cast<ItemId>(200)), std::out_of_range);
    EXPECT_THROW((void) mm6::spellForScroll(static_cast<ItemId>(300)), std::out_of_range);
}
