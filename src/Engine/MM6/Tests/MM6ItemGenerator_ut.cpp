#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ItemGenerator.h"

static std::unique_ptr<ItemTable> weaponTable() {
    auto table = std::make_unique<ItemTable>();
    auto add = [&](int id, ItemType type, Skill skill, int weight) {
        ItemData &data = table->items[static_cast<ItemId>(id)];
        data.type = type;
        data.skill = skill;
        data.uChanceByTreasureLvl[ITEM_TREASURE_LEVEL_4] = weight;
    };
    add(2, ITEM_TYPE_SINGLE_HANDED, SKILL_SWORD, 0);
    add(3, ITEM_TYPE_SINGLE_HANDED, SKILL_SWORD, 2);
    add(7, ITEM_TYPE_TWO_HANDED, SKILL_SWORD, 4);
    add(9, ITEM_TYPE_SINGLE_HANDED, SKILL_DAGGER, 5);
    add(20, ITEM_TYPE_TWO_HANDED, SKILL_CLUB, 1);
    add(21, ITEM_TYPE_TWO_HANDED, SKILL_STAFF, 1);
    add(250, ITEM_TYPE_SPELL_SCROLL, SKILL_MISC, 0);
    add(251, ITEM_TYPE_SPELL_SCROLL, SKILL_MISC, 0);
    table->items[static_cast<ItemId>(7)].identifyAndRepairDifficulty = 3;
    for (auto level : table->standardEnchantmentRangeByTreasureLevel.indices())
        table->standardEnchantmentRangeByTreasureLevel[level] = Segment(1, 1);
    return table;
}

UNIT_TEST(MM6ItemGenerator, FilteredKindDrawsOverTypeCandidatesOnly) {
    // Kind 20 asks for original item type 0. The two-handed item 7 is not a candidate, so the weights sum to 7.
    auto table = weaponTable();
    const std::array<std::uint8_t, 30> flags{};

    // Seed 1 draws 41, 41 % 7 = 6 lands on item 9 at cumulative 7.
    auto item = mm6::generateItem(GameTitle::MM6, *table, ITEM_TREASURE_LEVEL_4, RANDOM_ITEM_WEAPON, flags, 1);
    ASSERT_TRUE(item);
    EXPECT_EQ(item->itemId, 9);
    EXPECT_EQ(item->flags, 1u);
    EXPECT_EQ(item->drawsConsumed, 1);
    EXPECT_EQ(item->nextRngState, 2745024u);

    // Seed 100 draws 365, 365 % 7 = 1 lands on item 3 at cumulative 2.
    item = mm6::generateItem(GameTitle::MM6, *table, ITEM_TREASURE_LEVEL_4, RANDOM_ITEM_WEAPON, flags, 100);
    ASSERT_TRUE(item);
    EXPECT_EQ(item->itemId, 3);
    EXPECT_EQ(item->nextRngState, 23932311u);

    // Seed 12 draws 77, a zero remainder that picks the zero-weight first candidate.
    item = mm6::generateItem(GameTitle::MM6, *table, ITEM_TREASURE_LEVEL_4, RANDOM_ITEM_WEAPON, flags, 12);
    ASSERT_TRUE(item);
    EXPECT_EQ(item->itemId, 2);
    EXPECT_EQ(item->nextRngState, 5099167u);
}

UNIT_TEST(MM6ItemGenerator, SkillKindsUseOriginalSkillCodes) {
    // Kind 29 is the original club code 0 and kind 30 is staff, each with one weight-1 candidate.
    auto table = weaponTable();
    const std::array<std::uint8_t, 30> flags{};

    auto club = mm6::generateItem(GameTitle::MM6, *table, ITEM_TREASURE_LEVEL_4, RANDOM_ITEM_CLUB, flags, 0);
    ASSERT_TRUE(club);
    EXPECT_EQ(club->itemId, 20);
    EXPECT_EQ(club->drawsConsumed, 1);
    EXPECT_EQ(club->nextRngState, 2531011u);

    auto staff = mm6::generateItem(GameTitle::MM6, *table, ITEM_TREASURE_LEVEL_4, RANDOM_ITEM_STAFF, flags, 0);
    ASSERT_TRUE(staff);
    EXPECT_EQ(staff->itemId, 21);

    // Kind 23 is sword code 2, over items 2, 3 and 7 with weights 0, 2 and 4. Seed 1 draws 41, 41 % 6 = 5.
    auto sword = mm6::generateItem(GameTitle::MM6, *table, ITEM_TREASURE_LEVEL_4, RANDOM_ITEM_SWORD, flags, 1);
    ASSERT_TRUE(sword);
    EXPECT_EQ(sword->itemId, 7);
    EXPECT_EQ(sword->flags, 0u);
    EXPECT_EQ(sword->nextRngState, 2745024u);
}

UNIT_TEST(MM6ItemGenerator, ZeroWeightKindPicksFirstCandidateWithoutDrawing) {
    // Kind 43 is the original spell scroll type 15. All its candidates have zero weight, so nothing is drawn.
    auto table = weaponTable();
    const std::array<std::uint8_t, 30> flags{};
    auto scroll = mm6::generateItem(GameTitle::MM6, *table, ITEM_TREASURE_LEVEL_4, RANDOM_ITEM_SPELL_SCROLL, flags, 7);
    ASSERT_TRUE(scroll);
    EXPECT_EQ(scroll->itemId, 250);
    EXPECT_EQ(scroll->drawsConsumed, 0);
    EXPECT_EQ(scroll->nextRngState, 7u);

    // Kind 19 asks for type 18, gold, which has no candidates. The original would continue with item 0.
    EXPECT_FALSE(mm6::generateItem(GameTitle::MM6, *table, ITEM_TREASURE_LEVEL_4, static_cast<RandomItemType>(19), flags, 7));
    EXPECT_FALSE(mm6::generateItem(GameTitle::MM6, *table, ITEM_TREASURE_LEVEL_INVALID, RANDOM_ITEM_WEAPON, flags, 7));
    EXPECT_FALSE(mm6::generateItem(GameTitle::MM7, *table, ITEM_TREASURE_LEVEL_4, RANDOM_ITEM_WEAPON, flags, 7));
}

UNIT_TEST(MM6ItemGenerator, WeaponSpecialScansOnlyFiftyEightRows) {
    // A weapon draws its 100% special chance, then picks among level codes 0..2 at treasure level 4. Row 59 is loaded
    // but never scanned, so the eligible weights are 3 for special 2 and 2 for special 5.
    auto table = weaponTable();
    table->specialEnchantmentChanceForWeapons[ITEM_TREASURE_LEVEL_4] = 100;
    auto special = [&](int id, int code, int weight) {
        auto &row = table->specialEnchantments[static_cast<ItemEnchantment>(id)];
        row.enchantmentLevel = code;
        row.chanceByItemType[ITEM_TYPE_SINGLE_HANDED] = weight;
    };
    special(1, 3, 10);
    special(2, 1, 3);
    special(5, 2, 2);
    special(59, 1, 100);
    const std::array<std::uint8_t, 30> flags{};

    // Seed 1 draws 41 for item 9, 18467 % 100 = 67 for the chance and 6334 % 5 = 4 for special 5.
    auto item = mm6::generateItem(GameTitle::MM6, *table, ITEM_TREASURE_LEVEL_4, RANDOM_ITEM_WEAPON, flags, 1);
    ASSERT_TRUE(item);
    EXPECT_EQ(item->itemId, 9);
    EXPECT_EQ(item->specialEnchantmentId, 5);
    EXPECT_EQ(item->drawsConsumed, 3);
    EXPECT_EQ(item->nextRngState, 415139642u);

    // With only row 59 eligible the original would divide by zero.
    special(2, 3, 3);
    special(5, 3, 2);
    EXPECT_FALSE(mm6::generateItem(GameTitle::MM6, *table, ITEM_TREASURE_LEVEL_4, RANDOM_ITEM_WEAPON, flags, 1));
}

UNIT_TEST(MM6ItemGenerator, OnlyAnyKindRollsGeneratedArtifacts) {
    // Seed 17 at treasure level 6 draws artifact candidate 4 and a 2 for the 5% chance on the any-item path.
    auto table = std::make_unique<ItemTable>();
    table->items[static_cast<ItemId>(5)].type = ITEM_TYPE_SINGLE_HANDED;
    table->items[static_cast<ItemId>(5)].uChanceByTreasureLvl[ITEM_TREASURE_LEVEL_6] = 3;
    for (auto level : table->standardEnchantmentRangeByTreasureLevel.indices())
        table->standardEnchantmentRangeByTreasureLevel[level] = Segment(1, 1);
    std::array<std::uint8_t, 30> flags{};

    auto artifact = mm6::generateItem(GameTitle::MM6, *table, ITEM_TREASURE_LEVEL_6, RANDOM_ITEM_ANY, flags, 17);
    ASSERT_TRUE(artifact);
    EXPECT_EQ(artifact->itemId, 404);
    EXPECT_EQ(artifact->generatedArtifactFlags[4], 1);
    EXPECT_EQ(artifact->flags, 0u);
    EXPECT_EQ(artifact->nextRngState, 1743419155u);

    // The filtered path makes one draw over item 5 and leaves the artifact flags alone.
    auto weapon = mm6::generateItem(GameTitle::MM6, *table, ITEM_TREASURE_LEVEL_6, RANDOM_ITEM_WEAPON, flags, 17);
    ASSERT_TRUE(weapon);
    EXPECT_EQ(weapon->itemId, 5);
    EXPECT_EQ(weapon->drawsConsumed, 1);
    EXPECT_EQ(weapon->generatedArtifactFlags, flags);
}

UNIT_TEST(MM6ItemGenerator, TitleStagingMapsFieldsAndKeepsStateUntilPublished) {
    // The staged item carries runtime enums and the next LCG state, the input state is untouched.
    auto table = weaponTable();
    table->specialEnchantmentChanceForWeapons[ITEM_TREASURE_LEVEL_4] = 100;
    auto &row = table->specialEnchantments[static_cast<ItemEnchantment>(5)];
    row.enchantmentLevel = 2;
    row.chanceByItemType[ITEM_TYPE_SINGLE_HANDED] = 2;

    mm6::ChestTitleState state;
    EXPECT_THROW((void) mm6::stageTitleItem(state, *table, ITEM_TREASURE_LEVEL_4, RANDOM_ITEM_WEAPON), std::runtime_error);
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &state, 1, {}));
    auto staged = mm6::stageTitleItem(state, *table, ITEM_TREASURE_LEVEL_4, RANDOM_ITEM_WEAPON);
    EXPECT_EQ(staged.item.itemId, static_cast<ItemId>(9));
    EXPECT_EQ(staged.item.specialEnchantment, static_cast<ItemEnchantment>(5));
    EXPECT_EQ(staged.item.flags, ITEM_IDENTIFIED);
    EXPECT_EQ(staged.nextState.lcgState, 415139642u);
    EXPECT_EQ(state.lcgState, 1u);
    EXPECT_THROW((void) mm6::stageTitleItem(state, *table, ITEM_TREASURE_LEVEL_4, static_cast<RandomItemType>(19)),
                 std::runtime_error);

    mm6::ChestFinalizedItem fields;
    fields.itemId = 80;
    fields.standardAttributeIndex = 12;
    fields.standardStrength = 3;
    Item ring = mm6::generatedRuntimeItem(fields);
    EXPECT_EQ(ring.standardEnchantment, ATTRIBUTE_RESIST_AIR);
    EXPECT_EQ(ring.standardEnchantmentStrength, 3);
    EXPECT_EQ(ring.specialEnchantment, ITEM_ENCHANTMENT_NULL);
}
