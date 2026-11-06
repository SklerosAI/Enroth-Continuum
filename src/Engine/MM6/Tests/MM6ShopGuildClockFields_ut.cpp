#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ShopGuildClockFields.h"

UNIT_TEST(MM6ShopGuildClockFields, SourceProvenRowsShareBoundarySlot) {
    EXPECT_EQ(mm6::shopClockSlot(1), 0);
    EXPECT_EQ(mm6::shopClockSlot(47), 46);
    EXPECT_EQ(mm6::guildClockSlot(119), 46);
    EXPECT_EQ(mm6::guildClockSlot(140), 67);
    EXPECT_THROW((void) mm6::shopClockSlot(0), std::exception);
    EXPECT_THROW((void) mm6::shopClockSlot(48), std::exception);
    EXPECT_THROW((void) mm6::guildClockSlot(118), std::exception);
    EXPECT_THROW((void) mm6::guildClockSlot(141), std::exception);
}

UNIT_TEST(MM6ShopGuildClockFields, NonzeroClocksRoundTripAndLateFailureIsAtomic) {
    mm6::ShopGuildClockFields live;
    live.ready = true;
    live.deadlineWords[mm6::shopClockSlot(1)] = 123456;
    live.deadlineWords[mm6::shopClockSlot(47)] = 0x123456789abcdef0;
    live.deadlineWords[mm6::guildClockSlot(140)] = 789012;
    EXPECT_EQ(live.deadlineWords[mm6::guildClockSlot(119)], 0x123456789abcdef0);
    mm6::ShopGuildClockFields captured = mm6::captureShopGuildClocks(live, GameTitle::MM6);
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeShopGuildClockSection(captured));
    ASSERT_EQ(container.sections[0].payload.size(), 545);
    EXPECT_EQ(mm6::decodeShopGuildClockSection(container), live);

    mm6::ShopGuildClockFields prior;
    prior.ready = true;
    prior.deadlineWords[0] = 77;
    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(container);
    mm6::loadShopGuildClocksBeforeMutation(bytes, &prior, GameTitle::MM6);
    EXPECT_EQ(prior, live);
    prior.deadlineWords[0] = 77;
    container.sections[0].payload.pop_back(); // Final guild deadline byte is missing.
    EXPECT_THROW(mm6::loadShopGuildClocksBeforeMutation(
        mm6::encodeSaveContainer(container), &prior, GameTitle::MM6), std::exception);
    EXPECT_EQ(prior.deadlineWords[0], 77);
    EXPECT_EQ(prior.deadlineWords[mm6::guildClockSlot(140)], 789012);
    EXPECT_THROW(mm6::loadShopGuildClocksBeforeMutation(bytes, &prior, GameTitle::MM7), std::exception);
    EXPECT_EQ(prior.deadlineWords[0], 77);
    EXPECT_THROW((void) mm6::captureShopGuildClocks(live, GameTitle::MM7), std::exception);
    live.ready = false;
    EXPECT_THROW((void) mm6::captureShopGuildClocks(live, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::encodeShopGuildClockSection(live), std::exception);
}
