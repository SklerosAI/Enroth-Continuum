#include <array>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Tables/MM6PotionCombinations.h"
#include "Engine/MM6/MM6ConsumableRules.h"

namespace {

using Matrix = std::array<std::array<std::string, mm6::PotionCombinations::COUNT>, mm6::PotionCombinations::COUNT>;

Matrix exampleMatrix() {
    Matrix matrix;
    for (int row = 0; row < mm6::PotionCombinations::COUNT; ++row)
        for (int column = 0; column < mm6::PotionCombinations::COUNT; ++column)
            matrix[row][column] = row == column ? "QQ" : "Z1";
    matrix[0][1] = matrix[1][0] = "162";
    return matrix;
}

std::string encodeMatrix(const Matrix &matrix) {
    std::string text = "header\t\t\t\t\t";
    for (int column = 0; column < mm6::PotionCombinations::COUNT; ++column)
        text += "\t" + std::to_string(mm6::PotionCombinations::FIRST_ID + column);
    text += "\n";
    for (int row = 0; row < mm6::PotionCombinations::COUNT; ++row) {
        text += std::to_string(mm6::PotionCombinations::FIRST_ID + row) + "\t\t\t\t\t";
        for (const std::string &token : matrix[row])
            text += "\t" + token;
        text += "\n";
    }
    return text;
}

} // namespace

UNIT_TEST(MM6ConsumableRules, ClassifiesRetainedBandsAndRejectsMismatchedTypes) {
    for (int id = 160; id <= 196; ++id) {
        ItemId item = static_cast<ItemId>(id);
        ItemType expectedType = id <= 162 ? ITEM_TYPE_REAGENT : ITEM_TYPE_POTION;
        mm6::ConsumableKind expectedKind = id <= 162 ? mm6::ConsumableKind::Herb
            : id == 163 ? mm6::ConsumableKind::EmptyBottle
            : id <= 188 ? mm6::ConsumableKind::FilledBottle
            : mm6::ConsumableKind::UnsupportedBottle;
        EXPECT_EQ(mm6::classifyConsumable(item, expectedType), expectedKind);
        EXPECT_EQ(mm6::classifyConsumable(item, ITEM_TYPE_MISC), mm6::ConsumableKind::InvalidTemplate);
        EXPECT_TRUE(mm6::shouldBlockDirectUse(item, expectedType));
        EXPECT_TRUE(mm6::shouldBlockItemOnItemUse(item, expectedType));
    }
    EXPECT_EQ(mm6::classifyConsumable(static_cast<ItemId>(197), ITEM_TYPE_GOLD), mm6::ConsumableKind::Other);
    EXPECT_EQ(mm6::classifyConsumable(static_cast<ItemId>(159), ITEM_TYPE_WAND), mm6::ConsumableKind::Other);
    EXPECT_EQ(mm6::classifyConsumable(static_cast<ItemId>(200), ITEM_TYPE_POTION),
              mm6::ConsumableKind::InvalidTemplate);
}

UNIT_TEST(MM6ConsumableRules, BlocksInheritedPotionIdOverlapWithoutBlockingOtherItems) {
    for (int id = static_cast<int>(ITEM_FIRST_REAGENT); id <= static_cast<int>(ITEM_LAST_POTION); ++id) {
        ItemId item = static_cast<ItemId>(id);
        EXPECT_TRUE(mm6::shouldBlockItemOnItemUse(item, ITEM_TYPE_SPELL_SCROLL));
        EXPECT_FALSE(mm6::shouldBlockDirectUse(item, ITEM_TYPE_SPELL_SCROLL));
    }
    EXPECT_FALSE(mm6::shouldBlockItemOnItemUse(static_cast<ItemId>(159), ITEM_TYPE_WAND));
    EXPECT_FALSE(mm6::shouldBlockItemOnItemUse(static_cast<ItemId>(197), ITEM_TYPE_GOLD));
    EXPECT_FALSE(mm6::shouldBlockItemOnItemUse(static_cast<ItemId>(272), ITEM_TYPE_SPELL_SCROLL));
}

UNIT_TEST(MM6PotionCombinations, ParsesAndLooksUpEverySyntheticCell) {
    mm6::PotionCombinations unloaded;
    EXPECT_THROW((void) unloaded.lookup(static_cast<ItemId>(160), static_cast<ItemId>(161)), std::out_of_range);

    mm6::PotionCombinations table = mm6::PotionCombinations::parse(encodeMatrix(exampleMatrix()));
    ASSERT_TRUE(table.loaded());
    int products = 0;
    int markers = 0;
    for (int first = 160; first <= 188; ++first) {
        for (int second = 160; second <= 188; ++second) {
            mm6::PotionCombinationOutcome outcome = table.lookup(
                static_cast<ItemId>(first), static_cast<ItemId>(second));
            if (outcome.kind == mm6::PotionCombinationKind::Product) {
                ++products;
                EXPECT_EQ(outcome.product, static_cast<ItemId>(162));
            } else {
                ++markers;
                EXPECT_EQ(outcome.marker, first == second ? (std::array<char, 2>{'Q', 'Q'})
                                                         : (std::array<char, 2>{'Z', '1'}));
            }
        }
    }
    EXPECT_EQ(products, 2);
    EXPECT_EQ(markers, 839);
    EXPECT_THROW((void) table.lookup(static_cast<ItemId>(159), static_cast<ItemId>(160)), std::out_of_range);
    EXPECT_THROW((void) table.lookup(static_cast<ItemId>(160), static_cast<ItemId>(189)), std::out_of_range);
}

UNIT_TEST(MM6PotionCombinations, RejectsMalformedAxesCellsAndAsymmetry) {
    Matrix matrix = exampleMatrix();
    matrix[0][1] = "163";
    EXPECT_THROW((void) mm6::PotionCombinations::parse(encodeMatrix(matrix)), std::runtime_error);

    matrix = exampleMatrix();
    matrix[0][1] = matrix[1][0] = "999";
    EXPECT_THROW((void) mm6::PotionCombinations::parse(encodeMatrix(matrix)), std::runtime_error);

    matrix = exampleMatrix();
    matrix[0][1] = matrix[1][0] = "???";
    EXPECT_THROW((void) mm6::PotionCombinations::parse(encodeMatrix(matrix)), std::runtime_error);

    matrix = exampleMatrix();
    matrix[0][0] = "Q1";
    EXPECT_THROW((void) mm6::PotionCombinations::parse(encodeMatrix(matrix)), std::runtime_error);

    std::string text = encodeMatrix(exampleMatrix());
    EXPECT_THROW((void) mm6::PotionCombinations::parse(text.substr(0, text.rfind('\n', text.size() - 2))),
                 std::runtime_error);
    std::size_t firstEnd = text.find('\n');
    std::size_t secondEnd = text.find('\n', firstEnd + 1);
    std::string duplicate = text + text.substr(firstEnd + 1, secondEnd - firstEnd);
    EXPECT_THROW((void) mm6::PotionCombinations::parse(duplicate), std::runtime_error);
    EXPECT_THROW((void) mm6::PotionCombinations::parse(text.substr(firstEnd + 1)), std::runtime_error);
}

UNIT_TEST(MM6PotionCombinations, RetainedMatrixAggregateWhenSupplied) {
    const char *path = std::getenv("MM6_F3_RETAINED_USEITEMS");
    if (!path)
        GTEST_SKIP() << "Set MM6_F3_RETAINED_USEITEMS to the ignored F3-matched copy";
    std::ifstream input(path, std::ios::binary);
    ASSERT_TRUE(input);
    std::string text(std::istreambuf_iterator<char>{input}, {});
    mm6::PotionCombinations table = mm6::PotionCombinations::parse(text);
    int products = 0;
    for (int first = 160; first <= 188; ++first)
        for (int second = 160; second <= 188; ++second)
            products += table.lookup(static_cast<ItemId>(first), static_cast<ItemId>(second)).kind ==
                mm6::PotionCombinationKind::Product;
    EXPECT_EQ(products, 50);
}
