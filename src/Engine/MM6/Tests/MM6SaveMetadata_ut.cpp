#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6SaveMetadata.h"
#include "Engine/MM6/MM6CoreSaveFields.h"
#include "Engine/Graphics/Overlays.h"
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

UNIT_TEST(MM6SaveMetadata, CapturesAndStagesTimerAndFiftyOverlaysOnCopies) {
    Timer clock;
    clock.restorePausedSimulationTime(Duration::fromTicks(123456));
    ActiveOverlayList overlays;
    overlays.pOverlays[4].indexToOverlayTable = 3;
    overlays.pOverlays[4].spriteFrameTime = 120;
    overlays.pOverlays[4].animLength = 350;
    overlays.pOverlays[4].screenSpaceX = -17;
    overlays.pOverlays[4].screenSpaceY = 60;
    overlays.pOverlays[4].pid = Pid::actor(7);
    overlays.pOverlays[4].projSize = 22;
    overlays.pOverlays[4].fpDamageMod = 70000;

    mm6::SessionVisualFields captured = mm6::captureSessionVisualFields(clock, overlays, 5, GameTitle::MM6);
    EXPECT_EQ(captured.timer.elapsedTicks, 123456);
    EXPECT_EQ(captured.overlays[4].packedPid, Pid::actor(7).packed());
    EXPECT_EQ(captured.overlays[4].damageModifier, 70000);

    mm6::SaveMetadataFields metadata = exampleFields();
    metadata.timer = captured.timer;
    metadata.overlays = captured.overlays;
    mm6::SaveMetadataFields decoded = mm6::decodeMetadataSections(containerFor(metadata));
    Timer priorClock;
    ActiveOverlayList priorOverlays;
    priorOverlays.pOverlays[4].screenSpaceX = 99;
    mm6::StagedSessionVisuals staged = mm6::stageSessionVisuals(
        priorClock, priorOverlays, {decoded.timer, decoded.overlays}, 5, GameTitle::MM6);
    EXPECT_TRUE(staged.timer.isPaused());
    EXPECT_FALSE(staged.timer.isTurnBased());
    EXPECT_EQ(staged.timer.dt().ticks(), 0);
    EXPECT_EQ(staged.timer.time().ticks(), 123456);
    EXPECT_EQ(staged.overlays.pOverlays[4].screenSpaceX, -17);
    EXPECT_EQ(staged.overlays.pOverlays[4].pid, Pid::actor(7));
    EXPECT_EQ(priorClock.time().ticks(), 0);
    EXPECT_EQ(priorOverlays.pOverlays[4].screenSpaceX, 99);

    captured.overlays[49].animationLength = 1;
    captured.overlays[49].tableIndex = 5;
    EXPECT_THROW((void) mm6::stageSessionVisuals(priorClock, priorOverlays, captured, 5, GameTitle::MM6),
                 std::exception);
    EXPECT_THROW((void) mm6::captureSessionVisualFields(clock, overlays, 5, GameTitle::MM7), std::exception);
    EXPECT_EQ(priorOverlays.pOverlays[4].screenSpaceX, 99);
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
