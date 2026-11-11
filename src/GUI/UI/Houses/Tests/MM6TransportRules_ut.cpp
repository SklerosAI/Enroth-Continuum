#include <array>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "GUI/UI/Houses/MM6TransportRules.h"

#include "Library/Json/Json.h"

#include "Utility/Memory/Blob.h"

namespace {

std::string privateManifest() {
    const char *path = std::getenv("ENROTH_MM6_TRANSPORT_MANIFEST");
    if (!path)
        return {};
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("Cannot open ignored MM6 transport manifest fixture");
    return {std::istreambuf_iterator<char>{stream}, {}};
}

} // namespace

UNIT_TEST(MM6TransportRules, PrivateManifestShape) {
    std::string text = privateManifest();
    if (text.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_TRANSPORT_MANIFEST to the private profile manifest";

    mm6::TransportRules rules = mm6::decodeTransportRules(Blob::fromString(std::move(text)));
    ASSERT_EQ(rules.routes.size(), 36);
    ASSERT_EQ(rules.houses.size(), 21);
    for (const mm6::TransportHouse &house : rules.houses)
        EXPECT_EQ(rules.findHouse(house.id), &house);
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
    rules.houses.push_back({48, {{{mm6::TransportSlotKind::Scheduled, 0}, {mm6::TransportSlotKind::Blank, 0}, {mm6::TransportSlotKind::SpecialUnresolved, 2}}}});
    rules.houses.push_back({49, {{{mm6::TransportSlotKind::Scheduled, 1}, {mm6::TransportSlotKind::Scheduled, 0}, {mm6::TransportSlotKind::Scheduled, 1}}}});
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

UNIT_TEST(MM6TransportRules, RejectsAlteredPrivateManifest) {
    std::string text = privateManifest();
    if (text.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_TRANSPORT_MANIFEST to the private profile manifest";

    Json document = Json::parse(text);
    document["routes"][0]["map_index"] = 0;
    EXPECT_THROW(mm6::decodeTransportRules(Blob::fromString(document.dump())), std::exception);
    document = Json::parse(text);
    document["houses"][0]["slots"][0]["kind"] = "unverified";
    EXPECT_THROW(mm6::decodeTransportRules(Blob::fromString(document.dump())), std::exception);
    document = Json::parse(text);
    document["houses"][1]["id"] = document["houses"][0]["id"];
    EXPECT_THROW(mm6::decodeTransportRules(Blob::fromString(document.dump())), std::exception);
    document = Json::parse(text);
    document["houses"][0]["slots"][0]["route"] = 36;
    EXPECT_THROW(mm6::decodeTransportRules(Blob::fromString(document.dump())), std::exception);
}
