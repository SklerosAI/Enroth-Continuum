#include <algorithm>
#include <array>
#include <memory>

#include "Testing/Unit/MM6TestData.h"
#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6Executable.h"

#include "GUI/UI/Houses/MM6TransportRules.h"

UNIT_TEST(MM6TransportRules, ExecutableTables) {
    // 21 stables and boats with three slots each, every slot on one of 36 routes to a map in games.lod.
    const std::unique_ptr<MM6TestData> data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;
    const mm6::TransportRules rules = mm6::decodeTransportRules(mm6::Executable(data->read("mm6.exe")));
    ASSERT_EQ(rules.routes.size(), 36);
    ASSERT_EQ(rules.houses.size(), 21);
    for (const mm6::TransportHouse &house : rules.houses) {
        EXPECT_EQ(rules.findHouse(house.id), &house);
        for (size_t route : house.routes)
            EXPECT_LT(route, rules.routes.size());
    }
    for (const mm6::TransportRoute &route : rules.routes) {
        EXPECT_GE(route.mapIndex, 1);
        EXPECT_LE(route.mapIndex, data->games().ls().size());
        EXPECT_GE(route.duration, 1);
        EXPECT_NE(std::ranges::count(route.days, 1), 0);
    }
    const auto never = [](int) { return false; };
    EXPECT_EQ(rules.offeredRoutes(999, 0, never), (std::array<const mm6::TransportRoute *, 3>{}));
}

UNIT_TEST(MM6TransportRules, OfferedRoutes) {
    // A house whose first two slots share a route, and whose third route needs quest bit 7.
    mm6::TransportRules rules;
    rules.routes.resize(3);
    rules.routes[0].days = {1, 1, 0, 0, 0, 0, 0};
    rules.routes[1].days = {1, 0, 0, 0, 0, 0, 1};
    rules.routes[2].days = {1, 1, 1, 1, 1, 1, 1};
    rules.routes[2].questBit = 7;
    rules.houses.push_back({48, {0, 0, 2}});
    rules.houses.push_back({49, {1, 0, 1}});
    const auto never = [](int) { return false; };
    const auto bit7 = [](int bit) { return bit == 7; };
    using Offer = std::array<const mm6::TransportRoute *, 3>;
    const mm6::TransportRoute *r0 = &rules.routes[0];
    const mm6::TransportRoute *r1 = &rules.routes[1];
    const mm6::TransportRoute *r2 = &rules.routes[2];

    EXPECT_EQ(rules.offeredRoutes(48, 0, never), (Offer{r0, nullptr, nullptr}));
    EXPECT_EQ(rules.offeredRoutes(48, 0, bit7), (Offer{r0, nullptr, r2}));
    EXPECT_EQ(rules.offeredRoutes(48, 2, bit7), (Offer{nullptr, nullptr, r2}));
    EXPECT_EQ(rules.offeredRoutes(49, 0, never), (Offer{r1, r0, r1}));
    EXPECT_EQ(rules.offeredRoutes(49, 1, never), (Offer{nullptr, r0, nullptr}));
    EXPECT_EQ(rules.offeredRoutes(49, 6, never), (Offer{r1, nullptr, nullptr}));
    EXPECT_EQ(rules.offeredRoutes(50, 0, bit7), (Offer{}));
}

UNIT_TEST(MM6TransportRules, Price) {
    EXPECT_EQ(mm6::transportPrice(48, 1.0f, 0), 25);
    EXPECT_EQ(mm6::transportPrice(56, 10.0f, 0), 250);
    EXPECT_EQ(mm6::transportPrice(57, 2.0f, 0), 100);
    EXPECT_EQ(mm6::transportPrice(60, 5.0f, 10), 225);
    EXPECT_EQ(mm6::transportPrice(60, 5.0f, 90), 83); // A third of the full price, not of the base price.
    EXPECT_EQ(mm6::transportPrice(59, 10.0f, 0), 0);
}

UNIT_TEST(MM6TransportRules, DepartureWaitsForTheSpeech) {
    // House 48 is a stable, house 58 a boat.
    EXPECT_EQ(mm6::transportDepartureDelayMs(48, 0), 1500);
    EXPECT_EQ(mm6::transportDepartureDelayMs(58, 0), 2500);
    EXPECT_EQ(mm6::transportDepartureDelayMs(48, 700), 700);
    EXPECT_EQ(mm6::transportDepartureDelayMs(58, 2000), 2000);
    EXPECT_EQ(mm6::transportDepartureDelayMs(48, 2000), 1500);
    EXPECT_EQ(mm6::transportDepartureDelayMs(58, 4000), 2500);
}
