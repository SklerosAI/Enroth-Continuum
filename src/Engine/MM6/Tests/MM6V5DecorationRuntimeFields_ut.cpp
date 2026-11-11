#include <bit>
#include <cstdint>
#include <exception>
#include <memory>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6V5DecorationRuntimeFields.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Tables/DecorationTable.h"

namespace {

struct Fixture {
    std::unique_ptr<OutdoorLocation_MM7> base = std::make_unique<OutdoorLocation_MM7>();
    DecorationTable table;
    mm6::EventRuntimeMapEntry event;
    mm6::DecorationRuntimeSaveFields fields;

    Fixture() {
        base->decorations.resize(1);
        base->decorations[0].uDecorationDescID = 146;
        base->decorationNames.resize(1);
        base->decorationNames[0][0] = 'd';
        base->decorationNames[0][1] = 'e';
        base->decorationNames[0][2] = 'c';
        base->decorationNames[0][3] = '0';
        base->decorationNames[0][4] = '1';
        table.decorations.resize(147);
        table.decorations[146].internalName = "dec01";
        fields.mapName = "oute3.odm";
        fields.decorations.resize(1);
        reconstruct(base->decorations[0], &fields.decorations[0]);
        fields.decorations[0].uDecorationDescID = static_cast<DecorationId>(146);
        fields.decorations[0].eventVarId = 0;
        event.mapName = "oute3.odm";
        event.bindings = mm6::captureEventRuntimeDecorationBindings(
            fields.decorations, GameTitle::MM6);
    }
};

} // namespace

UNIT_TEST(MM6V5DecorationRuntimeFields, RoundTripsChangedFlagsAndBindsSourceSlot) {
    Fixture fixture;
    fixture.fields.decorations[0].uFlags |= LEVEL_DECORATION_VISIBLE_ON_MAP;
    fixture.fields.decorations[0].uFlags |= LEVEL_DECORATION_INVISIBLE;
    mm6::requireDecorationRuntimeBaseAgreement(
        fixture.fields, "oute3.odm", *fixture.base, fixture.event,
        fixture.table, GameTitle::MM6);
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeDecorationRuntimeSection(
        fixture.fields, GameTitle::MM6));
    const auto replay = mm6::decodeDecorationRuntimeSection(container, GameTitle::MM6);
    ASSERT_EQ(replay.decorations.size(), 1);
    EXPECT_EQ(replay.decorations[0].uFlags, fixture.fields.decorations[0].uFlags);
    EXPECT_EQ(mm6::encodeDecorationRuntimeSection(replay, GameTitle::MM6),
              container.sections[0]);
}

UNIT_TEST(MM6V5DecorationRuntimeFields, RefusesChangedSourceAnchorAndBadRuntimeSlot) {
    Fixture fixture;
    fixture.fields.decorations[0].vPosition.x = 1.5f;
    EXPECT_THROW((void) mm6::requireDecorationRuntimeBaseAgreement(
        fixture.fields, "oute3.odm", *fixture.base, fixture.event,
        fixture.table, GameTitle::MM6), std::exception);
    fixture.fields.decorations[0].vPosition.x = 0;
    fixture.fields.decorations[0].uDecorationDescID = static_cast<DecorationId>(200);
    EXPECT_THROW((void) mm6::requireDecorationRuntimeBaseAgreement(
        fixture.fields, "oute3.odm", *fixture.base, fixture.event,
        fixture.table, GameTitle::MM6), std::exception);
    fixture.fields.decorations[0].uDecorationDescID = static_cast<DecorationId>(146);
    fixture.fields.decorations[0].eventVarId = -76;
    EXPECT_THROW((void) mm6::encodeDecorationRuntimeSection(
        fixture.fields, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6V5DecorationRuntimeFields, KeepsUnassignedOrdinalBeyondFirst124) {
    Fixture fixture;
    fixture.base->decorations.resize(125);
    fixture.base->decorationNames.resize(125, fixture.base->decorationNames[0]);
    fixture.fields.decorations.resize(125);
    for (std::size_t index = 0; index < 125; ++index) {
        fixture.base->decorations[index].uDecorationDescID = 146;
        reconstruct(fixture.base->decorations[index], &fixture.fields.decorations[index]);
        fixture.fields.decorations[index].uDecorationDescID = static_cast<DecorationId>(146);
        if (index < 124)
            fixture.fields.decorations[index].eventVarId = index;
    }
    fixture.event.bindings = mm6::captureEventRuntimeDecorationBindings(
        fixture.fields.decorations, GameTitle::MM6);
    EXPECT_NO_THROW(mm6::requireDecorationRuntimeBaseAgreement(
        fixture.fields, "oute3.odm", *fixture.base, fixture.event,
        fixture.table, GameTitle::MM6));
    fixture.fields.decorations[0].eventVarId = -75;
    EXPECT_THROW((void) mm6::requireDecorationRuntimeBaseAgreement(
        fixture.fields, "oute3.odm", *fixture.base, fixture.event,
        fixture.table, GameTitle::MM6), std::exception);
    fixture.fields.decorations[0].eventVarId = 0;
    fixture.fields.decorations[124].eventVarId = 0;
    EXPECT_THROW((void) mm6::requireDecorationRuntimeBaseAgreement(
        fixture.fields, "oute3.odm", *fixture.base, fixture.event,
        fixture.table, GameTitle::MM6), std::exception);
}
