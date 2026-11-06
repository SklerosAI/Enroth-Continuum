#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "Engine/Data/HouseEnumFunctions.h"
#include "Engine/Tables/HouseTable.h"

#include "gtest/gtest.h"

TEST(MM6HouseService, CategoriesDoNotUseHouseIdRanges) {
    using mm6::HouseServiceFamily;
    EXPECT_EQ(mm6::houseServiceFamily(HOUSE_TYPE_GENERAL_STORE), HouseServiceFamily::Shop);
    EXPECT_EQ(mm6::houseServiceFamily(HOUSE_TYPE_ELEMENTAL_GUILD), HouseServiceFamily::MagicGuild);
    EXPECT_EQ(mm6::houseServiceFamily(HOUSE_TYPE_SELF_GUILD), HouseServiceFamily::MagicGuild);
    EXPECT_EQ(mm6::houseServiceFamily(HOUSE_TYPE_BOAT), HouseServiceFamily::Transport);
    EXPECT_EQ(mm6::houseServiceFamily(HOUSE_TYPE_MERCENARY_GUILD), HouseServiceFamily::Other);
    EXPECT_EQ(mm6::houseServiceFamily(HOUSE_TYPE_SHADOW_GUILD), HouseServiceFamily::Other);
    EXPECT_EQ(mm6::houseServiceFamily(HOUSE_TYPE_HOUSE), HouseServiceFamily::Other);
}

TEST(MM6HouseService, RetainedTableFamilyCounts) {
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!research)
        GTEST_SKIP() << "private retained MM6 table is unavailable";

    std::ifstream source(std::filesystem::path(research) / "icons" / "2devents.txt");
    ASSERT_TRUE(source.is_open());
    std::array<int, 4> counts{};
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
        ++counts[static_cast<int>(mm6::houseServiceFamily(type))];
    }
    EXPECT_EQ(counts[static_cast<int>(mm6::HouseServiceFamily::Shop)], 47);
    EXPECT_EQ(counts[static_cast<int>(mm6::HouseServiceFamily::MagicGuild)], 22);
    EXPECT_EQ(counts[static_cast<int>(mm6::HouseServiceFamily::Transport)], 21);
    EXPECT_EQ(counts[static_cast<int>(mm6::HouseServiceFamily::Other)], 467);
}
