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

UNIT_TEST(MM6TransportRules, PrivateManifestAndScheduleGates) {
    std::string text = privateManifest();
    if (text.empty())
        GTEST_SKIP() << "Set ENROTH_MM6_TRANSPORT_MANIFEST to the private profile manifest";

    mm6::TransportRules rules = mm6::decodeTransportRules(Blob::fromString(std::move(text)));
    ASSERT_EQ(rules.routes.size(), 36);
    ASSERT_EQ(rules.houses.size(), 21);
    size_t scheduled = 0, uncertain = 0, special = 0;
    for (const mm6::TransportHouse &house : rules.houses) {
        EXPECT_EQ(rules.findHouse(house.id), &house);
        for (size_t slot = 0; slot < house.slots.size(); ++slot) {
            const mm6::TransportSlot &choice = house.slots[slot];
            scheduled += choice.kind == mm6::TransportSlotKind::Scheduled;
            uncertain += choice.kind == mm6::TransportSlotKind::WeekdayUncertain;
            special += choice.kind == mm6::TransportSlotKind::SpecialUnresolved;
            for (int day = 0; day < 7; ++day) {
                const mm6::TransportRoute *route = rules.scheduledRoute(house.id, slot, day, false);
                const bool scheduledToday = choice.kind == mm6::TransportSlotKind::Scheduled &&
                                            rules.routes[choice.route].days[day];
                EXPECT_EQ(route != nullptr, scheduledToday && !rules.routes[choice.route].questBit);
                EXPECT_EQ(rules.scheduledRoute(house.id, slot, day, true) != nullptr, scheduledToday);
            }
        }
    }
    EXPECT_EQ(scheduled, 34);
    EXPECT_EQ(uncertain, 1);
    EXPECT_EQ(special, 2);
    EXPECT_EQ(rules.scheduledRoute(999, 0, 0, false), nullptr);
    EXPECT_EQ(rules.scheduledRoute(rules.houses[0].id, 3, 0, false), nullptr);
    EXPECT_EQ(rules.scheduledRoute(rules.houses[0].id, 0, 7, false), nullptr);
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
