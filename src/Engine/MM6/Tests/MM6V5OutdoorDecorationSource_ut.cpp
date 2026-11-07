#include <algorithm>
#include <array>
#include <exception>
#include <memory>
#include <string_view>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6V5DecorationRuntimeFields.h"
#include "Engine/MM6/MM6V5OutdoorDecorationSource.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Tables/DecorationTable.h"

namespace {

void setName(std::array<char, 32> *destination, std::string_view name) {
    destination->fill('\0');
    std::copy(name.begin(), name.end(), destination->begin());
}

} // namespace

UNIT_TEST(MM6V5OutdoorDecorationSource, BindsNativeNullOnlyToItsExactTableRow) {
    DecorationTable table;
    table.decorations.resize(2);
    table.decorations[0].internalName = "null00";
    table.decorations[1].internalName = "dec01";
    EXPECT_EQ(mm6::requireMM6OutdoorSourceDecorationId(0, "null00", table),
              DECORATION_NULL);
    EXPECT_EQ(mm6::requireMM6OutdoorSourceDecorationId(1, "dec01", table),
              static_cast<DecorationId>(1));
    EXPECT_THROW((void) mm6::requireMM6OutdoorSourceDecorationId(
        0, "unknown", table), std::exception);
    EXPECT_THROW((void) mm6::requireMM6OutdoorSourceDecorationId(
        1, "null00", table), std::exception);
}

UNIT_TEST(MM6V5OutdoorDecorationSource, V5JoinAdmitsOnlySourceBoundNullSlot) {
    DecorationTable table;
    table.decorations.resize(2);
    table.decorations[0].internalName = "null00";
    table.decorations[1].internalName = "dec01";
    auto base = std::make_unique<OutdoorLocation_MM7>();
    base->decorations.resize(1);
    base->decorationNames.resize(1);
    setName(&base->decorationNames[0], "null00");
    mm6::DecorationRuntimeSaveFields fields;
    fields.mapName = "oute2.odm";
    fields.decorations.resize(1);
    reconstruct(base->decorations[0], &fields.decorations[0]);
    fields.decorations[0].uDecorationDescID = DECORATION_NULL;
    mm6::EventRuntimeMapEntry event;
    event.mapName = fields.mapName;
    EXPECT_NO_THROW(mm6::requireDecorationRuntimeBaseAgreement(
        fields, "oute2.odm", *base, event, table, GameTitle::MM6));
    EXPECT_NO_THROW((void) mm6::encodeDecorationRuntimeSection(fields, GameTitle::MM6));
    setName(&base->decorationNames[0], "unknown");
    EXPECT_THROW(mm6::requireDecorationRuntimeBaseAgreement(
        fields, "oute2.odm", *base, event, table, GameTitle::MM6), std::exception);
    setName(&base->decorationNames[0], "dec01");
    base->decorations[0].uDecorationDescID = 1;
    EXPECT_THROW(mm6::requireDecorationRuntimeBaseAgreement(
        fields, "oute2.odm", *base, event, table, GameTitle::MM6), std::exception);
}
