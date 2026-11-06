#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>

#include "Engine/Tables/AwardTable.h"

#include "Utility/Memory/Blob.h"

#include "gtest/gtest.h"

namespace {

std::string syntheticAwards(int lastId = 100) {
    std::string text = "Index\tText\tPriority\r\n";
    for (int id = 1; id <= lastId; ++id)
        text += std::to_string(id) + "\tAward " + std::to_string(id) + "\r\n";
    return text;
}

} // namespace

TEST(MM6AwardTable, LoadsBlankPriorityRowsWithoutInventingPriority) {
    initializeAwards(Blob::fromString(syntheticAwards()), GameTitle::MM6);
    EXPECT_EQ(pAwards[static_cast<AwardId>(1)].pText, "Award 1");
    EXPECT_EQ(pAwards[static_cast<AwardId>(100)].pText, "Award 100");
    EXPECT_EQ(pAwards[static_cast<AwardId>(1)].uPriority, 0u);
    EXPECT_EQ(pAwards[static_cast<AwardId>(100)].uPriority, 0u);
    EXPECT_TRUE(pAwards[static_cast<AwardId>(101)].pText.empty());
}

TEST(MM6AwardTable, RejectsIncompleteAndInventedPriorityWithoutReplacingLoadedTable) {
    initializeAwards(Blob::fromString(syntheticAwards()), GameTitle::MM6);
    EXPECT_THROW(initializeAwards(Blob::fromString(syntheticAwards(99)), GameTitle::MM6), std::exception);
    EXPECT_EQ(pAwards[static_cast<AwardId>(100)].pText, "Award 100");

    std::string withPriority = syntheticAwards();
    withPriority.replace(withPriority.find("1\tAward 1\r\n"), std::string("1\tAward 1\r\n").size(),
                         "1\tAward 1\t7\r\n");
    EXPECT_THROW(initializeAwards(Blob::fromString(std::move(withPriority)), GameTitle::MM6), std::exception);
    EXPECT_EQ(pAwards[static_cast<AwardId>(1)].pText, "Award 1");
}

TEST(MM6AwardTable, PreservesMM7PriorityParser) {
    initializeAwards(Blob::fromString("Index\tText\tPriority\r\n1\tMM7 award\t3\r\n"), GameTitle::MM7);
    EXPECT_EQ(pAwards[static_cast<AwardId>(1)].pText, "MM7 award");
    EXPECT_EQ(pAwards[static_cast<AwardId>(1)].uPriority, 3u);
}

TEST(MM6AwardTable, RetainedHundredRowsLoadWithTextAndNoPriority) {
    const char *research = std::getenv("ENROTH_MM6_F3_RESEARCH_DIR");
    if (!research)
        GTEST_SKIP() << "private retained MM6 table is unavailable";

    std::ifstream source(std::filesystem::path(research) / "icons" / "awards.txt", std::ios::binary);
    ASSERT_TRUE(source.is_open());
    std::string bytes(std::istreambuf_iterator<char>{source}, {});
    initializeAwards(Blob::fromString(std::move(bytes)), GameTitle::MM6);

    int withText = 0;
    int deferredFormat = 0;
    for (int id = 1; id <= 100; ++id) {
        const AwardData &award = pAwards[static_cast<AwardId>(id)];
        withText += !award.pText.empty();
        deferredFormat += !canRenderAwardText(GameTitle::MM6, award.pText);
        EXPECT_EQ(award.uPriority, 0u);
    }
    EXPECT_EQ(withText, 86);
    EXPECT_EQ(deferredFormat, 7);
    EXPECT_TRUE(canRenderAwardText(GameTitle::MM7, "%u"));
}
