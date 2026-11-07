#include "MM6RawSessionStage.h"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>

#include "MM6AtomicSaveFile.h"
#include "MM6RawCursorShopStage.h"
#include "MM6SavedActorPidPolicy.h"

namespace mm6 {

std::optional<StagedRawSessionFields> preflightStagedRawSessionFields(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    const NpcStaticBaseline &freshNpcs, const Timer &freshTimer,
    const ActiveOverlayList &freshOverlays,
    std::span<const std::string> expectedNames, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, const ItemTable &itemTable,
    std::size_t overlayTableCount, std::size_t tileTableCount, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 raw session preflight requires the MM6 title");
    Party freshParty;
    std::optional<StagedRawSessionFields> staged;
    const auto selected = preflightRecoverableRawSaveFile(
        profileRoot, slotName, [&](std::span<const std::uint8_t> bytes) {
            StagedRawTitleOwners raw = stageRawTitleOwners(
                bytes, expectedNames, readBase, verifyItem, title);
            validateRawCursorShopJoin(raw.saved, raw.cursor, raw.shops, title);
            StagedRawPartyInventories party = stageRawPartyRepresentedFields(
                freshParty, raw, itemTable, title);
            StagedRawWorldAndVisuals world = stageRawWorldAndVisuals(
                freshNpcs, freshTimer, freshOverlays, raw, overlayTableCount, title);
            requireNoActiveSavedActorOverlays(raw.saved.metadata.overlays, title);
            DetachedPersistentEventBoundary persistentEvents =
                stagePersistentEventBoundary(raw.activeMap, title);
            std::optional<DetachedIndoorWorldPlan> indoorPlan;
            std::optional<DetachedOutdoorWorldPlan> outdoorPlan;
            if (std::holds_alternative<StagedOutdoorMap>(raw.activeMap))
                outdoorPlan = prepareDetachedRawOutdoorWorldPlan(raw, tileTableCount, title);
            else
                indoorPlan = prepareDetachedRawIndoorWorldPlan(raw, title);
            staged.emplace(StagedRawSessionFields{
                std::move(raw), std::move(party), std::move(world),
                std::move(persistentEvents),
                std::move(indoorPlan), std::move(outdoorPlan), false});
        });
    if (!selected)
        return std::nullopt;
    if (!staged)
        throw std::runtime_error("MM6 raw slot selected without staged session fields");
    staged->needsRecovery = selected->needsRecovery;
    return staged;
}

} // namespace mm6
