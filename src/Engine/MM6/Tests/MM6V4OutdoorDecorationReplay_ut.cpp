#include <exception>
#include <memory>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6V4OutdoorDecorationReplay.h"
#include "Engine/Snapshots/CompositeSnapshots.h"
#include "Engine/Tables/DecorationTable.h"

namespace {

struct Fixture {
    std::unique_ptr<OutdoorLocation_MM7> base = std::make_unique<OutdoorLocation_MM7>();
    DecorationTable table;
    mm6::EventRuntimeMapEntry events;
    std::vector<LevelDecoration> live;

    Fixture() {
        base->decorations.resize(1);
        base->decorationNames.resize(1);
        base->decorationNames[0][0] = 'd';
        base->decorationNames[0][1] = 'e';
        base->decorationNames[0][2] = 'c';
        base->decorationNames[0][3] = '0';
        base->decorationNames[0][4] = '1';
        table.decorations.resize(147);
        table.decorations[146].internalName = "dec01";
        live.resize(1);
        reconstruct(base->decorations[0], &live[0]);
        live[0].uDecorationDescID = static_cast<DecorationId>(146);
        events.mapName = "oute3.odm";
        events.bindings = mm6::captureEventRuntimeDecorationBindings(live, GameTitle::MM6);
        live[0].eventVarId = 0;
        live[0].uFlags |= LEVEL_DECORATION_INVISIBLE;
    }

    void check() const {
        mm6::requireFirstMapOutdoorDecorationReplay(
            "oute3.odm", *base, events, table, live, GameTitle::MM6);
    }
};

} // namespace

UNIT_TEST(MM6V4OutdoorDecorationReplay, RebuildsVisibilityAndRejectsFlagMutation) {
    Fixture fixture;
    EXPECT_NO_THROW(fixture.check());
    fixture.live[0].uFlags = {};
    EXPECT_THROW(fixture.check(), std::exception);
    fixture.live[0].uFlags |= LEVEL_DECORATION_INVISIBLE;
    fixture.events.decorVars[0] = 1;
    EXPECT_THROW(fixture.check(), std::exception);
}

UNIT_TEST(MM6V4OutdoorDecorationReplay, RejectsBindingIdentityAndOtherMaps) {
    Fixture fixture;
    fixture.live[0].vPosition.x = 12;
    EXPECT_THROW(fixture.check(), std::exception);
    fixture.live[0].vPosition.x = 0;
    fixture.events.bindings[0].decorationIndex = 1;
    EXPECT_THROW(fixture.check(), std::exception);
    fixture.events.bindings[0].decorationIndex = 0;
    EXPECT_THROW((void) mm6::requireFirstMapOutdoorDecorationReplay(
        "oute4.odm", *fixture.base, fixture.events, fixture.table,
        fixture.live, GameTitle::MM6), std::exception);
}
