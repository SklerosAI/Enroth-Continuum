#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ChestSaveFields.h"
#include "Engine/MM6/MM6RawChildPublicationForTest.h"
#include "Engine/MM6/MM6RawSessionStage.h"
#include "Engine/Objects/Inventory.h"

namespace {

std::unique_ptr<mm6::StagedRawSessionFields> candidate() {
    auto staged = std::make_unique<mm6::StagedRawSessionFields>();
    staged->raw.inventory.ready = true;
    staged->raw.saved.inventory = staged->raw.inventory;
    staged->party.raw = staged->raw.inventory;
    for (Character &character : staged->party.party.pCharacters)
        character.inventory = CharacterInventory::forMM6();
    staged->raw.cursor = *mm6::freshRawHeldCursorOwner(GameTitle::MM6);
    staged->raw.saved.heldItem = *mm6::captureRawHeldCursor(staged->raw.cursor, GameTitle::MM6);
    staged->raw.shops = mm6::emptyNewGameShopTitleState(GameTitle::MM6);
    staged->raw.saved.shops = staged->raw.shops;

    staged->raw.saved.metadata.header.mapFileName = "synthetic0.blv";
    staged->indoorPlan.emplace();
    staged->indoorPlan->mapName = "synthetic0.blv";
    mm6::StagedIndoorMap active;
    active.mapName = "synthetic0.blv";
    std::array<std::uint8_t, 200> persistent{};
    active.delta.wire.sections.push_back({MM6IndoorSectionKind::PersistentBytes,
        Blob::copy(persistent.data(), persistent.size()), 0});
    staged->raw.activeMap = std::move(active);

    const std::array<std::uint8_t, 3> delta = {1, 2, 3};
    staged->raw.saved.maps.deltas.push_back({"synthetic0.dlv", {delta.begin(), delta.end()}});
    staged->world.world.mapDeltas.emplace("synthetic0.dlv", Blob::copy(delta.data(), delta.size()));
    staged->raw.saved.npcs = mm6::captureNpcFields(staged->world.world.npcs, GameTitle::MM6);
    const auto visuals = mm6::captureSessionVisualFields(
        staged->world.visuals.timer, staged->world.visuals.overlays, 5, GameTitle::MM6);
    staged->raw.saved.metadata.timer = visuals.timer;
    staged->raw.saved.metadata.overlays = visuals.overlays;
    staged->world.chestTitle.ready = true;
    staged->world.rawActiveDelta = {1};
    staged->raw.saved.chest = mm6::captureChestSaveFields(
        staged->world.chestTitle, "synthetic0.dlv", staged->world.rawActiveDelta, GameTitle::MM6);
    return staged;
}

bool hasBlocker(const mm6::testing::RawChildPublicationAdmission &admission,
                mm6::testing::RawChildPublicationBlocker blocker) {
    return std::find(admission.blockers.begin(), admission.blockers.end(), blocker) !=
        admission.blockers.end();
}

} // namespace

UNIT_TEST(MM6RawChildPublicationForTest, CoherentRepresentedOwnersRemainBlockedBeforeWorldAndUi) {
    auto staged = candidate();
    ItemTable items;
    const auto admission = mm6::testing::inspectRawChildPublication(
        *staged, items, GameTitle::MM6);
    EXPECT_FALSE(admission.canEnterGameLoop());
    EXPECT_FALSE(admission.cursorImage.occupied);
    EXPECT_TRUE(hasBlocker(admission,
        mm6::testing::RawChildPublicationBlocker::ActiveWorldResourcesNotBound));
    EXPECT_TRUE(hasBlocker(admission,
        mm6::testing::RawChildPublicationBlocker::PersistentEventsNotCheckedAfterWorldLoad));
    EXPECT_FALSE(hasBlocker(admission,
        mm6::testing::RawChildPublicationBlocker::HeldItemOffsetUnresolved));
    staged->raw.activeCounts.chests = 1;
    staged->needsRecovery = true;
    const auto changed = mm6::testing::inspectRawChildPublication(*staged, items, GameTitle::MM6);
    EXPECT_TRUE(hasBlocker(changed,
        mm6::testing::RawChildPublicationBlocker::RebuiltObjectIdentityUnchecked));
    EXPECT_TRUE(hasBlocker(changed,
        mm6::testing::RawChildPublicationBlocker::BackupNeedsRecovery));
}

UNIT_TEST(MM6RawChildPublicationForTest, RejectsLateOwnerDivergenceBeforeAdmission) {
    auto staged = candidate();
    ItemTable items;
    staged->world.world.mapDeltas.at("synthetic0.dlv") = Blob::copy("bad", 3);
    EXPECT_THROW((void) mm6::testing::inspectRawChildPublication(*staged, items, GameTitle::MM6),
                 std::exception);
    staged = candidate();
    staged->world.chestTitle.lcgState = 17;
    EXPECT_THROW((void) mm6::testing::inspectRawChildPublication(*staged, items, GameTitle::MM6),
                 std::exception);
    staged = candidate();
    staged->raw.shops.clocks.deadlineWords[0] = 1;
    EXPECT_THROW((void) mm6::testing::inspectRawChildPublication(*staged, items, GameTitle::MM6),
                 std::exception);
    staged = candidate();
    staged->persistentEvents.mapBytes[0] = 1;
    EXPECT_THROW((void) mm6::testing::inspectRawChildPublication(*staged, items, GameTitle::MM6),
                 std::exception);
    staged = candidate();
    EXPECT_THROW((void) mm6::testing::inspectRawChildPublication(*staged, items, GameTitle::MM7),
                 std::exception);
}
