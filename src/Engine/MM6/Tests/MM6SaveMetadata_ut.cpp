#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6SaveMetadata.h"
#include "Engine/MM6/MM6CoreSaveFields.h"
#include "Engine/Timer.h"

namespace {

mm6::SaveMetadataFields exampleFields() {
    mm6::SaveMetadataFields fields;
    fields.header.displayName = "A New World";
    fields.header.mapFileName = "oute3.odm";
    fields.header.playingTicks = 987654;
    fields.timer.elapsedTicks = 12345;
    fields.ui.activeCharacter = 2;
    fields.overlays[4].tableIndex = 3;
    fields.overlays[4].frameTime = 120;
    fields.overlays[4].animationLength = 350;
    fields.overlays[4].x = -17;
    fields.overlays[4].y = 60;
    fields.overlays[4].packedPid = 0x102;
    fields.overlays[4].projectedSize = 22;
    fields.overlays[4].damageModifier = 65536;
    return fields;
}

mm6::SaveContainer containerFor(const mm6::SaveMetadataFields &fields) {
    auto sections = mm6::encodeMetadataSections(fields);
    mm6::SaveContainer container;
    container.sections.assign(sections.begin(), sections.end());
    return container;
}

} // namespace

UNIT_TEST(MM6SaveMetadata, HeaderTimeUiAndOverlayRoundTrip) {
    mm6::SaveMetadataFields source = exampleFields();
    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(containerFor(source));
    EXPECT_EQ(mm6::decodeMetadataSections(mm6::decodeSaveContainer(bytes)), source);

    mm6::SaveMetadataFields destination;
    mm6::loadMetadataBeforeMutation(bytes, &destination);
    EXPECT_EQ(destination, source);
}

UNIT_TEST(MM6SaveMetadata, RejectionDoesNotMutateDestination) {
    mm6::SaveMetadataFields destination = exampleFields();
    mm6::SaveMetadataFields before = destination;
    mm6::SaveContainer invalid = containerFor(before);
    invalid.sections[0].payload[2 + before.header.displayName.size() + 1] = '/'; // Map name byte.
    EXPECT_THROW(mm6::loadMetadataBeforeMutation(mm6::encodeSaveContainer(invalid), &destination), std::exception);
    EXPECT_EQ(destination, before);

    invalid = containerFor(before);
    invalid.sections[1].payload[8] = 0xff; // Negative elapsed time, valid container CRC.
    EXPECT_THROW(mm6::loadMetadataBeforeMutation(mm6::encodeSaveContainer(invalid), &destination), std::exception);
    EXPECT_EQ(destination, before);

    std::vector<std::uint8_t> wrongTitle = mm6::encodeSaveContainer(containerFor(before));
    wrongTitle[2] = '7';
    EXPECT_THROW(mm6::loadMetadataBeforeMutation(wrongTitle, &destination), std::exception);
    EXPECT_EQ(destination, before);
}

UNIT_TEST(MM6SaveMetadata, RequiresEverySectionAndValidPolicy) {
    mm6::SaveContainer missing = containerFor(exampleFields());
    missing.sections.pop_back();
    EXPECT_THROW((void) mm6::decodeMetadataSections(missing), std::exception);

    mm6::SaveMetadataFields invalid = exampleFields();
    invalid.ui.resetTransientUi = 0;
    EXPECT_THROW((void) mm6::encodeMetadataSections(invalid), std::exception);
    invalid = exampleFields();
    invalid.overlays[4].tableIndex = -1;
    EXPECT_THROW((void) mm6::encodeMetadataSections(invalid), std::exception);
}

UNIT_TEST(MM6SaveMetadata, TimerRestoreDiscardsFrameTiming) {
    Timer timer;
    timer.restorePausedSimulationTime(Duration::fromTicks(123456));
    EXPECT_TRUE(timer.isPaused());
    EXPECT_FALSE(timer.isTurnBased());
    EXPECT_EQ(timer.dt().ticks(), 0);
    EXPECT_EQ(timer.time().ticks(), 123456);
}

UNIT_TEST(MM6SaveMetadata, CrossSectionClockAndSelectionMustAgree) {
    mm6::SaveMetadataFields metadata = exampleFields();
    mm6::CoreSaveFields core;
    core.party.playingTicks = metadata.header.playingTicks;
    core.party.activeCharacter = metadata.ui.activeCharacter;
    EXPECT_NO_THROW(mm6::validateMetadataAgainstCore(metadata, core));
    core.party.activeCharacter = 1;
    EXPECT_THROW(mm6::validateMetadataAgainstCore(metadata, core), std::exception);
    core.party.activeCharacter = metadata.ui.activeCharacter;
    core.party.playingTicks++;
    EXPECT_THROW(mm6::validateMetadataAgainstCore(metadata, core), std::exception);
}
