#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "Engine/Tables/HouseTable.h"

#include "gtest/gtest.h"

TEST(MM6HouseType, RoutesServicesAndEntrancesWithoutMercenaryFallback) {
    EXPECT_EQ(parseHouseType("General Store", GameTitle::MM6), HOUSE_TYPE_GENERAL_STORE);
    EXPECT_EQ(parseHouseType("Merc Guild", GameTitle::MM6), HOUSE_TYPE_MERCENARY_GUILD);
    EXPECT_EQ(parseHouseType("Thieves Guild", GameTitle::MM6), HOUSE_TYPE_SHADOW_GUILD);
    EXPECT_EQ(parseHouseType("Circus", GameTitle::MM6), HOUSE_TYPE_CIRCUS);
    EXPECT_EQ(parseHouseType("Jail", GameTitle::MM6), HOUSE_TYPE_JAIL);
    EXPECT_EQ(parseHouseType("The Seer", GameTitle::MM6), HOUSE_TYPE_SEER);
    EXPECT_EQ(parseHouseType("Dungeon Ent", GameTitle::MM6), HOUSE_TYPE_DUNGEON);
    EXPECT_EQ(parseHouseType("Temple Ent", GameTitle::MM6), HOUSE_TYPE_HOUSE);
    EXPECT_EQ(parseHouseType("Wagon", GameTitle::MM6), HOUSE_TYPE_HOUSE);
    EXPECT_EQ(parseHouseType("House P15", GameTitle::MM6), HOUSE_TYPE_HOUSE);
    EXPECT_EQ(parseHouseType("Hosue M9", GameTitle::MM6), HOUSE_TYPE_HOUSE);
    EXPECT_EQ(parseHouseType("House R9", GameTitle::MM6), HOUSE_TYPE_HOUSE);
    EXPECT_EQ(parseHouseType("Not Used", GameTitle::MM6), HOUSE_TYPE_INVALID);
    EXPECT_EQ(parseHouseType("Unknown Mod House", GameTitle::MM6), HOUSE_TYPE_INVALID);
}

TEST(MM6HouseType, PreservesMM7ParserFallback) {
    EXPECT_EQ(parseHouseType("Weapon Shop", GameTitle::MM7), HOUSE_TYPE_WEAPON_SHOP);
    EXPECT_EQ(parseHouseType("Unrecognized", GameTitle::MM7), HOUSE_TYPE_MERCENARY_GUILD);
    EXPECT_EQ(parseHouseType("", GameTitle::MM7), HOUSE_TYPE_INVALID);
}

TEST(MM6HouseType, RetainedTableHasNoUnknownLabels) {
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!research)
        GTEST_SKIP() << "private retained MM6 table is unavailable";

    std::ifstream source(std::filesystem::path(research) / "icons" / "2devents.txt");
    ASSERT_TRUE(source.is_open());
    int rows = 0;
    int unknown = 0;
    for (std::string line; std::getline(source, line);) {
        const size_t first = line.find('\t');
        if (first == std::string::npos || first == 0 ||
            !std::all_of(line.begin(), line.begin() + first,
                         [](char c) { return c >= '0' && c <= '9'; }))
            continue;
        const size_t second = line.find('\t', first + 1);
        ASSERT_NE(second, std::string::npos);
        const size_t third = line.find('\t', second + 1);
        ASSERT_NE(third, std::string::npos);
        const std::string_view label(line.data() + second + 1, third - second - 1);
        const HouseType type = parseHouseType(label, GameTitle::MM6);
        if (!label.empty() && label != "Not Used" && type == HOUSE_TYPE_INVALID)
            ++unknown;
        ++rows;
    }
    EXPECT_EQ(rows, 557);
    EXPECT_EQ(unknown, 0);
}
