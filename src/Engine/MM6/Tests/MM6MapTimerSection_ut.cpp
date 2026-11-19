#include <cstdint>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6MapTimerSection.h"

UNIT_TEST(MM6MapTimerSection, RoundTripsEveryMapAndPeriod) {
    mm6::MapTimerFirings firings;
    firings["d02.blv"] = {Time(), Time(), Time::fromDays(3), Time()};
    firings["oute3.odm"] = {Time::fromYears(1), Time::fromDays(28), Time::fromDays(7) + Duration::fromHours(5), Time::fromTicks(123456789)};
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeMapTimerSection(firings, GameTitle::MM6));

    EXPECT_FALSE(container.sections[0].required);
    EXPECT_EQ(mm6::decodeMapTimerSection(mm6::decodeSaveContainer(mm6::encodeSaveContainer(container)), GameTitle::MM6), firings);
}

UNIT_TEST(MM6MapTimerSection, SaveWithoutTheSectionHasNoFirings) {
    EXPECT_TRUE(mm6::decodeMapTimerSection(mm6::SaveContainer(), GameTitle::MM6).empty());
}

UNIT_TEST(MM6MapTimerSection, RefusesMalformedSections) {
    mm6::MapTimerFirings firings;
    firings["oute3.odm"] = {Time(), Time(), Time(), Time::fromDays(1)};
    const mm6::SaveSection valid = mm6::encodeMapTimerSection(firings, GameTitle::MM6);

    mm6::SaveContainer truncated;
    truncated.sections.push_back(valid);
    truncated.sections[0].payload.pop_back();
    EXPECT_ANY_THROW((void) mm6::decodeMapTimerSection(truncated, GameTitle::MM6));

    mm6::SaveContainer duplicate;
    duplicate.sections = {valid, valid};
    EXPECT_ANY_THROW((void) mm6::decodeMapTimerSection(duplicate, GameTitle::MM6));

    mm6::SaveContainer badName;
    badName.sections.push_back(valid);
    badName.sections[0].payload[3] = 'O';
    EXPECT_ANY_THROW((void) mm6::decodeMapTimerSection(badName, GameTitle::MM6));

    mm6::MapTimerFirings uppercase;
    uppercase["OUTE3.ODM"] = {};
    EXPECT_ANY_THROW((void) mm6::encodeMapTimerSection(uppercase, GameTitle::MM6));
    EXPECT_ANY_THROW((void) mm6::encodeMapTimerSection(firings, GameTitle::MM7));
}
