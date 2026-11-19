#include <array>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Testing/Unit/MM6TestData.h"
#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6Executable.h"

#include "GUI/UI/Houses/MM6ShopStock.h"

#include "Library/Random/SequentialRandomEngine.h"

#include "Utility/String/Format.h"

static mm6::ShopStockRules shopRules() {
    // Synthetic rules, weapon kinds in the 20s, armor in the 30s, and general stores with one generated choice.
    mm6::ShopStockRules result;
    result.visibleSlots = {6, 8, 12, 6};
    for (int bank = 0; bank < 2; ++bank) {
        for (int house = 1; house <= 47; ++house) {
            std::vector<mm6::ShopStockRow> &rows = result.banks[bank][house].rows;
            if (house < 15) {
                rows.push_back({1 + bank, {20, 21, 22, 23}});
            } else if (house < 29) {
                rows.push_back({2, {30, 31, 32, 33}});
                rows.push_back({3, {34, 35, 36, 37}});
            } else if (house < 42) {
                rows.push_back({4, {}});
            } else {
                rows.push_back({5, {0, 160, 161, 162, 163, 164}});
            }
        }
    }
    for (int house = 119; house <= 140; ++house)
        result.guildBooks[house - 119] = house < 133 ? 7 : 4;
    return result;
}

static std::pair<mm6::ShopItemGenerator, std::shared_ptr<std::vector<std::pair<int, int>>>> recordingGenerator() {
    // Writes the kind as the item id, so a test can see which kind each slot got.
    auto calls = std::make_shared<std::vector<std::pair<int, int>>>();
    mm6::ShopItemGenerator generator = [calls](int level, int kind, mm6::ItemRecord28 *record) {
        calls->emplace_back(level, kind);
        *record = {};
        record->setWord(0, kind);
    };
    return {generator, calls};
}

static mm6::ShopRandom random(SequentialRandomEngine &rng) {
    return [&rng](int hi) { return rng.random(hi); };
}

static int itemAt(const mm6::ShopStockFields &stock, mm6::ShopStockBank bank, int house, int slot) {
    return stock.records[mm6::shopStockRecordIndex(bank, house, slot, GameTitle::MM6)].itemNumberRaw();
}

UNIT_TEST(MM6ShopStock, ExecutableTables) {
    // Both banks hold all 47 shops with the row shape of their shop family, and every spell guild offers books.
    const std::unique_ptr<MM6TestData> data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;
    const mm6::ShopStockRules rules = mm6::decodeShopStockRules(mm6::Executable(data->read("mm6.exe")));
    EXPECT_EQ(rules.visibleSlots, (std::array<int, 4>{6, 8, 12, 6}));
    for (int bank = 0; bank < 2; ++bank) {
        for (int house = 1; house <= 47; ++house) {
            SCOPED_TRACE(fmt::format("bank={} house={}", bank, house));
            const std::vector<mm6::ShopStockRow> &rows = rules.banks[bank][house].rows;
            ASSERT_EQ(rows.size(), house >= 15 && house < 29 ? 2 : 1);
            for (const mm6::ShopStockRow &row : rows) {
                EXPECT_GE(row.treasureLevel, 1);
                EXPECT_LE(row.treasureLevel, 6);
                EXPECT_EQ(row.choices.size(), house < 29 ? 4 : house < 42 ? 0 : 6);
            }
        }
    }
    for (int books : rules.guildBooks) {
        EXPECT_GE(books, 1);
        EXPECT_LE(books, 11);
    }
}

UNIT_TEST(MM6ShopStock, WeaponAndArmorShopsPickKinds) {
    // A weapon shop picks one of four kinds per slot, and an armor shop switches to its second row from slot 4.
    const mm6::ShopStockRules rules = shopRules();
    auto [generator, calls] = recordingGenerator();
    SequentialRandomEngine rng;
    mm6::ShopStockFields stock;
    stock.records[mm6::shopStockRecordIndex(mm6::ShopStockBank::First, 1, 6, GameTitle::MM6)].setWord(0, 99);

    mm6::generateShopBank(&stock, rules, mm6::ShopStockBank::First, 1, HOUSE_TYPE_WEAPON_SHOP, random(rng), generator);
    EXPECT_EQ(*calls, (std::vector<std::pair<int, int>>{{1, 21}, {1, 22}, {1, 23}, {1, 20}, {1, 21}, {1, 22}}));
    EXPECT_EQ(itemAt(stock, mm6::ShopStockBank::First, 1, 0), 21);
    EXPECT_EQ(stock.records[mm6::shopStockRecordIndex(mm6::ShopStockBank::First, 1, 0, GameTitle::MM6)].conditionRaw(), 1);
    EXPECT_EQ(itemAt(stock, mm6::ShopStockBank::First, 1, 6), 99); // Past the visible slots.

    calls->clear();
    mm6::generateShopBank(&stock, rules, mm6::ShopStockBank::Second, 15, HOUSE_TYPE_ARMOR_SHOP, random(rng), generator);
    EXPECT_EQ(*calls, (std::vector<std::pair<int, int>>{{2, 33}, {2, 30}, {2, 31}, {2, 32}, {3, 37}, {3, 34}, {3, 35}, {3, 36}}));
}

UNIT_TEST(MM6ShopStock, GeneralStoreUsesDirectItems) {
    // A general store slot takes one of six choices, an item id directly or 0 for a generated kind 37 or 39 item.
    const mm6::ShopStockRules rules = shopRules();
    auto [generator, calls] = recordingGenerator();
    SequentialRandomEngine rng;
    mm6::ShopStockFields stock;
    stock.records[mm6::shopStockRecordIndex(mm6::ShopStockBank::First, 42, 0, GameTitle::MM6)].back() = 7;

    mm6::generateShopBank(&stock, rules, mm6::ShopStockBank::First, 42, HOUSE_TYPE_GENERAL_STORE, random(rng), generator);
    EXPECT_EQ(itemAt(stock, mm6::ShopStockBank::First, 42, 0), 160);
    EXPECT_EQ(stock.records[mm6::shopStockRecordIndex(mm6::ShopStockBank::First, 42, 0, GameTitle::MM6)].back(), 0);
    EXPECT_EQ(itemAt(stock, mm6::ShopStockBank::First, 42, 4), 164);
    EXPECT_EQ(*calls, (std::vector<std::pair<int, int>>{{5, 37}}));
    EXPECT_EQ(itemAt(stock, mm6::ShopStockBank::First, 42, 5), 37);
}

UNIT_TEST(MM6ShopStock, GeneratorFailurePropagates) {
    // A slot the random item generator can't fill aborts the bank, so that a refresh can roll back.
    const mm6::ShopStockRules rules = shopRules();
    SequentialRandomEngine rng;
    mm6::ShopStockFields stock;
    const mm6::ShopItemGenerator failing = [](int, int, mm6::ItemRecord28 *) { throw std::runtime_error("no item"); };
    EXPECT_THROW(mm6::generateShopBank(&stock, rules, mm6::ShopStockBank::First, 29, HOUSE_TYPE_MAGIC_SHOP, random(rng), failing),
                 std::runtime_error);
}

UNIT_TEST(MM6ShopStock, LastShopSecondBankIsFirstGuildRow) {
    // The last shop's second bank and the first spell guild share their physical stock records.
    const mm6::ShopStockRules rules = shopRules();
    auto [generator, calls] = recordingGenerator();
    SequentialRandomEngine rng;
    mm6::ShopStockFields stock;
    mm6::generateShopBank(&stock, rules, mm6::ShopStockBank::Second, 47, HOUSE_TYPE_GENERAL_STORE, random(rng), generator);
    for (int slot = 0; slot < 6; ++slot)
        EXPECT_EQ(stock.records[mm6::guildStockRecordIndex(119, slot, GameTitle::MM6)].itemNumberRaw(),
                  itemAt(stock, mm6::ShopStockBank::Second, 47, slot));
    EXPECT_NE(itemAt(stock, mm6::ShopStockBank::Second, 47, 0), 0);
}

UNIT_TEST(MM6ShopStock, GuildOffersBooksOfItsSchools) {
    // A one-school guild offers that school's first books, and a guild of several schools picks a school per slot.
    const mm6::ShopStockRules rules = shopRules();
    SequentialRandomEngine rng;
    mm6::ShopStockFields stock;
    mm6::ItemRecord28 &first = stock.records[mm6::guildStockRecordIndex(119, 0, GameTitle::MM6)];
    first.setWord(8, 5);

    mm6::generateGuildStock(&stock, rules, 119, HOUSE_TYPE_AIR_GUILD, random(rng));
    std::vector<int> books;
    for (int slot = 0; slot < 12; ++slot)
        books.push_back(stock.records[mm6::guildStockRecordIndex(119, slot, GameTitle::MM6)].itemNumberRaw());
    EXPECT_EQ(books, (std::vector<int>{312, 313, 314, 315, 316, 317, 311, 312, 313, 314, 315, 316}));
    EXPECT_EQ(first.conditionRaw(), 1);
    EXPECT_EQ(first.bonusStrengthRaw(), 5); // Untouched.

    books.clear();
    mm6::generateGuildStock(&stock, rules, 133, HOUSE_TYPE_SELF_GUILD, random(rng));
    for (int slot = 0; slot < 4; ++slot)
        books.push_back(stock.records[mm6::guildStockRecordIndex(133, slot, GameTitle::MM6)].itemNumberRaw());
    EXPECT_EQ(books, (std::vector<int>{357, 344, 368, 355}));
}
