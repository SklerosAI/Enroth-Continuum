#include <array>
#include <cstdint>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestSaveFields.h"

UNIT_TEST(MM6ChestSaveFields, NonzeroCaptureRoundTripAndDetachedRestore) {
    mm6::ChestTitleState live;
    live.ready = true;
    live.lcgState = 0xdeadbeef;
    live.generatedArtifactFlags[1] = 1;
    live.generatedArtifactFlags[29] = 1;
    std::vector<std::uint8_t> sidecar = {4, 3, 2, 1};
    mm6::ChestSaveFields fields = mm6::captureChestSaveFields(
        live, "oute3.ddm", sidecar, GameTitle::MM6);
    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeChestSection(fields));
    mm6::ChestSaveFields decoded = mm6::decodeChestSection(container);
    EXPECT_EQ(decoded, fields);

    mm6::ChestTitleState target;
    target.ready = true;
    target.lcgState = 9;
    std::vector<std::uint8_t> oldSidecar = {8, 8};
    mm6::restoreChestFieldsBeforeMutation(decoded, &target, &oldSidecar, GameTitle::MM6);
    EXPECT_TRUE(target.ready);
    EXPECT_EQ(target.lcgState, 0xdeadbeef);
    EXPECT_EQ(target.generatedArtifactFlags[29], 1);
    EXPECT_EQ(oldSidecar, sidecar);
    mm6::resetChestTitleState(&target);
    EXPECT_FALSE(target.ready);
    EXPECT_EQ(target.lcgState, 0);
    EXPECT_EQ(target.generatedArtifactFlags[29], 0);
}

UNIT_TEST(MM6ChestSaveFields, LateFailuresLeaveBothDetachedOwnersUnchanged) {
    mm6::ChestTitleState prior;
    prior.ready = true;
    prior.lcgState = 77;
    prior.generatedArtifactFlags[4] = 1;
    std::vector<std::uint8_t> priorSidecar = {9, 8, 7};
    mm6::ChestSaveFields source{0x12345678, {}, "oute3.ddm", {1, 2, 3}};
    source.generatedArtifactFlags[29] = 1;
    auto unchanged = [&] {
        EXPECT_TRUE(prior.ready);
        EXPECT_EQ(prior.lcgState, 77);
        EXPECT_EQ(prior.generatedArtifactFlags[4], 1);
        EXPECT_EQ(priorSidecar, (std::vector<std::uint8_t>{9, 8, 7}));
    };

    source.generatedArtifactFlags[29] = 2;
    EXPECT_THROW(mm6::restoreChestFieldsBeforeMutation(source, &prior, &priorSidecar,
                                                        GameTitle::MM6), std::exception);
    unchanged();
    source.generatedArtifactFlags[29] = 1;
    source.rawActiveDelta.clear();
    EXPECT_THROW(mm6::restoreChestFieldsBeforeMutation(source, &prior, &priorSidecar,
                                                        GameTitle::MM6), std::exception);
    unchanged();
    source.rawActiveDelta = {1, 2, 3};
    EXPECT_THROW(mm6::restoreChestFieldsBeforeMutation(source, &prior, &priorSidecar,
                                                        GameTitle::MM7), std::exception);
    unchanged();

    mm6::SaveContainer container;
    container.sections.push_back(mm6::encodeChestSection(source));
    container.sections.front().payload.back() = 0xff;
    EXPECT_NO_THROW((void) mm6::decodeChestSection(container)); // Opaque sidecar bytes are preserved.
    container.sections.front().payload[2 + source.activeDeltaName.size() + 4 + 29] = 2;
    EXPECT_THROW((void) mm6::decodeChestSection(container), std::exception);
    unchanged();
}
