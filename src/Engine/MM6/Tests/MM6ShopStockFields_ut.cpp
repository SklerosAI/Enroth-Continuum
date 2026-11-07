#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ShopStockFields.h"
#include "Engine/MM6/MM6ShopStockIdentity.h"
#include "Engine/MM6/MM6ShopRefreshBoundary.h"
#include "Engine/MM6/MM6ShopTitleState.h"

UNIT_TEST(MM6ShopTitleState, StagesStockAndClockTogetherBeforePublication) {
    mm6::ShopTitleState &live = mm6::shopTitleState();
    const mm6::ShopTitleState prior = live;
    struct Restore {
        mm6::ShopTitleState &live;
        mm6::ShopTitleState prior;
        ~Restore() { live = std::move(prior); }
    } restore{live, prior};

    mm6::resetShopTitleState(&live);
    ASSERT_FALSE(live.ready());
    EXPECT_THROW((void) mm6::emptyNewGameShopTitleState(GameTitle::MM7), std::exception);
    mm6::ShopTitleState seed = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    ASSERT_TRUE(seed.ready());
    const std::size_t shopIndex = mm6::shopStockRecordIndex(
        mm6::ShopStockBank::Second, 47, 5, GameTitle::MM6);
    ASSERT_EQ(shopIndex, mm6::guildStockRecordIndex(119, 5, GameTitle::MM6));
    const std::size_t clockIndex = mm6::shopClockSlot(47);
    ASSERT_EQ(clockIndex, mm6::guildClockSlot(119));
    seed.stock.records[shopIndex][0] = 73;
    seed.stock.records[shopIndex][27] = 0xa5;
    seed.clocks.deadlineWords[clockIndex] = 900;

    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeShopStockSection(seed.stock));
    container.sections.push_back(mm6::encodeShopGuildClockSection(seed.clocks));
    const std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(container);
    mm6::ShopTitleState staged = mm6::stageShopTitleState(bytes, GameTitle::MM6);
    ASSERT_EQ(staged, seed);
    mm6::publishShopTitleState(&live, std::move(staged), GameTitle::MM6);
    ASSERT_EQ(live, seed);
    EXPECT_EQ(live.stock.records[mm6::guildStockRecordIndex(119, 5, GameTitle::MM6)][27], 0xa5);
    EXPECT_EQ(live.clocks.deadlineWords[mm6::guildClockSlot(119)], 900);

    container.sections[1].payload.pop_back(); // Late SCLK failure.
    EXPECT_THROW((void) mm6::stageShopTitleState(mm6::encodeSaveContainer(container),
                                                  GameTitle::MM6), std::exception);
    EXPECT_EQ(live, seed);
    container.sections[1] = mm6::encodeShopGuildClockSection(seed.clocks);
    container.sections[0].payload.pop_back(); // Large SHOP image fails before commit.
    EXPECT_THROW((void) mm6::stageShopTitleState(mm6::encodeSaveContainer(container),
                                                  GameTitle::MM6), std::exception);
    EXPECT_EQ(live, seed);
    EXPECT_THROW((void) mm6::stageShopTitleState(bytes, GameTitle::MM7), std::exception);
    EXPECT_THROW(mm6::publishShopTitleState(&live, seed, GameTitle::MM7), std::exception);
    EXPECT_EQ(live, seed);
    mm6::resetShopTitleState(&live);
    EXPECT_FALSE(live.ready());
    EXPECT_EQ(live.stock.records[shopIndex][0], 0);
    EXPECT_EQ(live.clocks.deadlineWords[clockIndex], 0);
}

UNIT_TEST(MM6ShopStockFields, PhysicalAliasAndTailRoundTrip) {
    auto live = std::make_unique<mm6::ShopStockFields>();
    live->ready = true;
    const std::size_t shop = mm6::shopStockRecordIndex(mm6::ShopStockBank::Second, 47, 5, GameTitle::MM6);
    const std::size_t guild = mm6::guildStockRecordIndex(119, 5, GameTitle::MM6);
    ASSERT_EQ(shop, guild);
    for (std::size_t i = 0; i < 28; ++i)
        live->records[shop][i] = static_cast<std::uint8_t>(i + 1);
    live->records.back()[27] = 0xa5; // Preserve initialized physical rows beyond active guild keys.

    const mm6::ShopStockFields captured = mm6::captureShopStock(*live, GameTitle::MM6);
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeShopStockSection(captured));
    EXPECT_EQ(container.sections[0].payload.size(), 1 + 120 * 12 * 28);
    const std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(container);
    const mm6::ShopStockFields decoded = mm6::decodeShopStockSection(mm6::decodeSaveContainer(bytes));
    EXPECT_EQ(decoded, *live);
    EXPECT_EQ(decoded.records[guild][0], 1);
    EXPECT_EQ(decoded.records.back()[27], 0xa5);
}

UNIT_TEST(MM6ShopStockFields, RejectsMalformedOrForeignBeforeMutation) {
    auto live = std::make_unique<mm6::ShopStockFields>();
    live->ready = true;
    live->records[0][0] = 71;
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeShopStockSection(*live));
    const std::vector<std::uint8_t> valid = mm6::encodeSaveContainer(container);
    auto prior = std::make_unique<mm6::ShopStockFields>();
    prior->ready = true;
    prior->records[0][0] = 9;

    EXPECT_THROW(mm6::loadShopStockBeforeMutation(valid, prior.get(), GameTitle::MM7), std::exception);
    EXPECT_EQ(prior->records[0][0], 9);
    EXPECT_THROW((void) mm6::captureShopStock(*live, GameTitle::MM7), std::exception);
    live->ready = false;
    EXPECT_THROW((void) mm6::captureShopStock(*live, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::encodeShopStockSection(*live), std::exception);

    container.sections[0].payload.pop_back(); // Valid container checksum, short final record.
    EXPECT_THROW(mm6::loadShopStockBeforeMutation(mm6::encodeSaveContainer(container),
                                                 prior.get(), GameTitle::MM6), std::exception);
    EXPECT_EQ(prior->records[0][0], 9);
    container.sections[0].payload.push_back(0);
    container.sections[0].payload[0] = 2; // Unsupported SHOP section version.
    EXPECT_THROW(mm6::loadShopStockBeforeMutation(mm6::encodeSaveContainer(container),
                                                 prior.get(), GameTitle::MM6), std::exception);
    EXPECT_EQ(prior->records[0][0], 9);
    container.sections[0].payload[0] = 1;
    container.sections[0].required = false;
    EXPECT_THROW(mm6::loadShopStockBeforeMutation(mm6::encodeSaveContainer(container),
                                                 prior.get(), GameTitle::MM6), std::exception);
    EXPECT_EQ(prior->records[0][0], 9);
}

UNIT_TEST(MM6ShopStockFields, SharedDeadlineArbitratesObservedShopAndGuildRefresh) {
    auto stock = std::make_unique<mm6::ShopStockFields>();
    stock->ready = true;
    mm6::ShopGuildClockFields clocks;
    clocks.ready = true;
    const auto sharedStock = mm6::guildStockRecordIndex(119, 0, GameTitle::MM6);
    const auto sharedClock = mm6::shopClockSlot(47);
    ASSERT_EQ(sharedClock, mm6::guildClockSlot(119));
    ASSERT_EQ(sharedStock, mm6::shopStockRecordIndex(mm6::ShopStockBank::Second, 47, 0, GameTitle::MM6));

    int firstRuns = 0;
    int secondRuns = 0;
    int guildRuns = 0;
    const mm6::ShopStockGenerator first = [&](mm6::ShopStockFields &staged) {
        ++firstRuns;
        staged.records[mm6::shopStockRecordIndex(mm6::ShopStockBank::First, 47, 0, GameTitle::MM6)][0] = 31;
    };
    const mm6::ShopStockGenerator second = [&](mm6::ShopStockFields &staged) {
        ++secondRuns;
        staged.records[sharedStock][0] = 73;
    };
    const mm6::ShopStockGenerator guild = [&](mm6::ShopStockFields &staged) {
        ++guildRuns;
        staged.records[sharedStock][0] = 109;
    };

    EXPECT_FALSE(mm6::refreshShopStockIfExpired(stock.get(), &clocks, 47, 0, 100,
                                                first, second, GameTitle::MM6));
    EXPECT_TRUE(mm6::refreshShopStockIfExpired(stock.get(), &clocks, 47, 1, 100,
                                               first, second, GameTitle::MM6));
    EXPECT_EQ(firstRuns, 1);
    EXPECT_EQ(secondRuns, 1);
    EXPECT_EQ(stock->records[sharedStock][0], 73);
    EXPECT_EQ(clocks.deadlineWords[sharedClock], 100);
    EXPECT_FALSE(mm6::refreshGuildStockIfExpired(stock.get(), &clocks, 119, 1, 200,
                                                 guild, GameTitle::MM6));
    EXPECT_FALSE(mm6::refreshGuildStockIfExpired(stock.get(), &clocks, 119, 100, 200,
                                                 guild, GameTitle::MM6));
    EXPECT_EQ(guildRuns, 0);
    EXPECT_EQ(stock->records[sharedStock][0], 73);
    EXPECT_TRUE(mm6::refreshGuildStockIfExpired(stock.get(), &clocks, 119, 101, 200,
                                                guild, GameTitle::MM6));
    EXPECT_EQ(guildRuns, 1);
    EXPECT_EQ(stock->records[sharedStock][0], 109);
    EXPECT_FALSE(mm6::refreshShopStockIfExpired(stock.get(), &clocks, 47, 101, 300,
                                                first, second, GameTitle::MM6));
    EXPECT_EQ(firstRuns, 1);
    EXPECT_EQ(secondRuns, 1);
    clocks.deadlineWords[sharedClock] = 0x8000000000000000ull; // Original compares high words signed.
    EXPECT_TRUE(mm6::refreshShopStockIfExpired(stock.get(), &clocks, 47, 201, 300,
                                               first, second, GameTitle::MM6));
    EXPECT_EQ(secondRuns, 2);
}

UNIT_TEST(MM6ShopStockFields, FailedRefreshDoesNotPartiallyChangeDetachedOwners) {
    auto stock = std::make_unique<mm6::ShopStockFields>();
    stock->ready = true;
    mm6::ShopGuildClockFields clocks;
    clocks.ready = true;
    stock->records[0][0] = 11;
    const mm6::ShopStockGenerator first = [](mm6::ShopStockFields &staged) { staged.records[0][0] = 73; };
    const mm6::ShopStockGenerator fail = [](mm6::ShopStockFields &) {
        throw std::runtime_error("synthetic second-bank failure");
    };

    EXPECT_THROW(mm6::refreshShopStockIfExpired(stock.get(), &clocks, 47, 1, 100,
                                                first, fail, GameTitle::MM6), std::runtime_error);
    EXPECT_EQ(stock->records[0][0], 11);
    EXPECT_EQ(clocks.deadlineWords[mm6::shopClockSlot(47)], 0);
    EXPECT_THROW(mm6::refreshShopStockIfExpired(stock.get(), &clocks, 47, 1, 100,
                                                first, first, GameTitle::MM7), std::runtime_error);
    EXPECT_THROW(mm6::refreshShopStockIfExpired(stock.get(), &clocks, 47, 1, 1,
                                                first, first, GameTitle::MM6), std::runtime_error);
    EXPECT_EQ(stock->records[0][0], 11);
    EXPECT_EQ(clocks.deadlineWords[mm6::shopClockSlot(47)], 0);
}

UNIT_TEST(MM6ShopStockFields, EntryModRefreshesEachVisitAndPreservesSharedRow) {
    EXPECT_EQ(mm6::shopRefreshPolicyFromModValue("1", GameTitle::MM6),
              mm6::ShopRefreshPolicy::OnEntry);
    EXPECT_EQ(mm6::shopRefreshPolicyFromModValue("", GameTitle::MM6),
              mm6::ShopRefreshPolicy::WhenExpired);
    EXPECT_EQ(mm6::shopRefreshPolicyFromModValue("true", GameTitle::MM6),
              mm6::ShopRefreshPolicy::WhenExpired);
    EXPECT_EQ(mm6::shopRefreshPolicyFromModValue("1", GameTitle::MM7),
              mm6::ShopRefreshPolicy::WhenExpired);

    auto stock = std::make_unique<mm6::ShopStockFields>();
    stock->ready = true;
    mm6::ShopGuildClockFields clocks;
    clocks.ready = true;
    const auto sharedStock = mm6::guildStockRecordIndex(119, 0, GameTitle::MM6);
    const auto sharedClock = mm6::shopClockSlot(47);
    ASSERT_EQ(sharedClock, mm6::guildClockSlot(119));

    int firstRuns = 0;
    int secondRuns = 0;
    int guildRuns = 0;
    const mm6::ShopStockGenerator first = [&](mm6::ShopStockFields &) { ++firstRuns; };
    const mm6::ShopStockGenerator second = [&](mm6::ShopStockFields &staged) {
        staged.records[sharedStock][0] = static_cast<std::uint8_t>(++secondRuns);
    };
    const mm6::ShopStockGenerator guild = [&](mm6::ShopStockFields &staged) {
        staged.records[sharedStock][0] = static_cast<std::uint8_t>(100 + ++guildRuns);
    };

    EXPECT_TRUE(mm6::refreshShopStockForEntry(stock.get(), &clocks, 47, 1, 100,
                                               first, second, GameTitle::MM6,
                                               mm6::ShopRefreshPolicy::OnEntry));
    EXPECT_EQ(stock->records[sharedStock][0], 1);
    EXPECT_EQ(clocks.deadlineWords[sharedClock], 100);
    EXPECT_FALSE(mm6::refreshShopStockForEntry(stock.get(), &clocks, 47, 2, 200,
                                                first, second, GameTitle::MM6,
                                                mm6::ShopRefreshPolicy::WhenExpired));
    EXPECT_EQ(firstRuns, 1);
    EXPECT_EQ(secondRuns, 1);
    EXPECT_TRUE(mm6::refreshShopStockForEntry(stock.get(), &clocks, 47, 2, 200,
                                               first, second, GameTitle::MM6,
                                               mm6::ShopRefreshPolicy::OnEntry));
    EXPECT_EQ(firstRuns, 2);
    EXPECT_EQ(secondRuns, 2);
    EXPECT_EQ(stock->records[sharedStock][0], 2);
    EXPECT_EQ(clocks.deadlineWords[sharedClock], 200);

    EXPECT_TRUE(mm6::refreshGuildStockForEntry(stock.get(), &clocks, 119, 3, 300,
                                                guild, GameTitle::MM6,
                                                mm6::ShopRefreshPolicy::OnEntry));
    EXPECT_EQ(guildRuns, 1);
    EXPECT_EQ(stock->records[sharedStock][0], 101);
    EXPECT_EQ(clocks.deadlineWords[sharedClock], 300);
    EXPECT_TRUE(mm6::refreshShopStockForEntry(stock.get(), &clocks, 47, 4, 400,
                                               first, second, GameTitle::MM6,
                                               mm6::ShopRefreshPolicy::OnEntry));
    EXPECT_EQ(firstRuns, 3);
    EXPECT_EQ(secondRuns, 3);
    EXPECT_EQ(stock->records[sharedStock][0], 3);
    EXPECT_EQ(clocks.deadlineWords[sharedClock], 400);
}

UNIT_TEST(MM6ShopStockFields, EntryModGeneratorFailureLeavesBothOwnersUnchanged) {
    auto stock = std::make_unique<mm6::ShopStockFields>();
    stock->ready = true;
    stock->records[0][0] = 17;
    mm6::ShopGuildClockFields clocks;
    clocks.ready = true;
    clocks.deadlineWords[mm6::shopClockSlot(47)] = 500;
    const auto beforeStock = *stock;
    const auto beforeClocks = clocks;
    const mm6::ShopStockGenerator first = [](mm6::ShopStockFields &staged) {
        staged.records[0][0] = 42;
    };
    const mm6::ShopStockGenerator fail = [](mm6::ShopStockFields &) {
        throw std::runtime_error("synthetic generator failure");
    };

    EXPECT_THROW(mm6::refreshShopStockForEntry(stock.get(), &clocks, 47, 1, 600,
                                                first, fail, GameTitle::MM6,
                                                mm6::ShopRefreshPolicy::OnEntry), std::runtime_error);
    EXPECT_EQ(*stock, beforeStock);
    EXPECT_EQ(clocks, beforeClocks);
    EXPECT_THROW(mm6::refreshGuildStockForEntry(stock.get(), &clocks, 119, 1, 600,
                                                 fail, GameTitle::MM6,
                                                 mm6::ShopRefreshPolicy::OnEntry), std::runtime_error);
    EXPECT_EQ(*stock, beforeStock);
    EXPECT_EQ(clocks, beforeClocks);
    EXPECT_THROW(mm6::refreshShopStockForEntry(stock.get(), &clocks, 47, 1, 600,
                                                first, first, GameTitle::MM7,
                                                mm6::ShopRefreshPolicy::OnEntry), std::runtime_error);
    EXPECT_THROW(mm6::refreshShopStockForEntry(stock.get(), &clocks, 47, 1, 600,
                                                first, first, GameTitle::MM6,
                                                static_cast<mm6::ShopRefreshPolicy>(99)), std::runtime_error);
    EXPECT_EQ(*stock, beforeStock);
    EXPECT_EQ(clocks, beforeClocks);
}

UNIT_TEST(MM6ShopStockFields, EntryVisitGuardRerollsOncePerShopWindow) {
    auto stock = std::make_unique<mm6::ShopStockFields>();
    stock->ready = true;
    mm6::ShopGuildClockFields clocks;
    clocks.ready = true;
    int firstRuns = 0;
    int secondRuns = 0;
    const mm6::ShopStockGenerator first = [&](mm6::ShopStockFields &) { ++firstRuns; };
    const mm6::ShopStockGenerator second = [&](mm6::ShopStockFields &) { ++secondRuns; };

    mm6::ShopRefreshVisit firstVisit;
    EXPECT_FALSE(firstVisit.refreshShopOnEntry(stock.get(), &clocks, 47, 1, 100,
                                              first, second, GameTitle::MM6,
                                              mm6::ShopRefreshPolicy::WhenExpired));
    EXPECT_TRUE(firstVisit.refreshShopOnEntry(stock.get(), &clocks, 47, 1, 100,
                                             first, second, GameTitle::MM6,
                                             mm6::ShopRefreshPolicy::OnEntry));
    EXPECT_FALSE(firstVisit.refreshShopOnEntry(stock.get(), &clocks, 47, 2, 200,
                                              first, second, GameTitle::MM6,
                                              mm6::ShopRefreshPolicy::OnEntry));
    EXPECT_EQ(firstRuns, 1);
    EXPECT_EQ(secondRuns, 1);
    EXPECT_EQ(clocks.deadlineWords[mm6::shopClockSlot(47)], 100);

    mm6::ShopRefreshVisit nextVisit;
    EXPECT_TRUE(nextVisit.refreshShopOnEntry(stock.get(), &clocks, 47, 2, 200,
                                            first, second, GameTitle::MM6,
                                            mm6::ShopRefreshPolicy::OnEntry));
    EXPECT_EQ(firstRuns, 2);
    EXPECT_EQ(secondRuns, 2);
    EXPECT_EQ(clocks.deadlineWords[mm6::shopClockSlot(47)], 200);
}

UNIT_TEST(MM6ShopStockFields, EntryVisitGuardRetriesFailedGuildAndRejectsForeignTitle) {
    auto stock = std::make_unique<mm6::ShopStockFields>();
    stock->ready = true;
    mm6::ShopGuildClockFields clocks;
    clocks.ready = true;
    mm6::ShopRefreshVisit visit;
    const mm6::ShopStockGenerator fail = [](mm6::ShopStockFields &) {
        throw std::runtime_error("synthetic guild generation failure");
    };
    int calls = 0;
    const mm6::ShopStockGenerator succeed = [&](mm6::ShopStockFields &) { ++calls; };
    EXPECT_THROW(visit.refreshGuildOnEntry(stock.get(), &clocks, 119, 1, 100,
                                           fail, GameTitle::MM6,
                                           mm6::ShopRefreshPolicy::OnEntry), std::runtime_error);
    EXPECT_EQ(clocks.deadlineWords[mm6::guildClockSlot(119)], 0);
    EXPECT_THROW(visit.refreshGuildOnEntry(stock.get(), &clocks, 119, 1, 100,
                                           succeed, GameTitle::MM7,
                                           mm6::ShopRefreshPolicy::OnEntry), std::runtime_error);
    EXPECT_TRUE(visit.refreshGuildOnEntry(stock.get(), &clocks, 119, 1, 100,
                                          succeed, GameTitle::MM6,
                                          mm6::ShopRefreshPolicy::OnEntry));
    EXPECT_FALSE(visit.refreshGuildOnEntry(stock.get(), &clocks, 119, 2, 200,
                                           succeed, GameTitle::MM6,
                                           mm6::ShopRefreshPolicy::OnEntry));
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(clocks.deadlineWords[mm6::guildClockSlot(119)], 100);
}
