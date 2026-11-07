#include "MM6RawSessionStage.h"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <utility>

#include "MM6AtomicSaveFile.h"

namespace mm6 {

std::optional<StagedRawSessionFields> preflightStagedRawSessionFields(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    const Party &freshParty, const NPCStats &freshNpcs, const Timer &freshTimer,
    const ActiveOverlayList &freshOverlays,
    std::span<const std::string> expectedNames, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, const ItemTable &itemTable,
    std::size_t overlayTableCount, std::size_t tileTableCount, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 raw session preflight requires the MM6 title");
    std::optional<StagedRawSessionFields> staged;
    const auto selected = preflightRecoverableRawSaveFile(
        profileRoot, slotName, [&](std::span<const std::uint8_t> bytes) {
            StagedRawTitleOwners raw = stageRawTitleOwners(
                bytes, expectedNames, readBase, verifyItem, title);
            StagedRawPartyInventories party = stageRawPartyRepresentedFields(
                freshParty, raw, itemTable, title);
            StagedRawWorldAndVisuals world = stageRawWorldAndVisuals(
                freshNpcs, freshTimer, freshOverlays, raw, overlayTableCount, title);
            std::optional<DetachedOutdoorWorldPlan> outdoorPlan;
            if (std::holds_alternative<StagedOutdoorMap>(raw.activeMap))
                outdoorPlan = prepareDetachedRawOutdoorWorldPlan(raw, tileTableCount, title);
            staged.emplace(StagedRawSessionFields{
                std::move(raw), std::move(party), std::move(world),
                std::move(outdoorPlan), false});
        });
    if (!selected)
        return std::nullopt;
    if (!staged)
        throw std::runtime_error("MM6 raw slot selected without staged session fields");
    staged->needsRecovery = selected->needsRecovery;
    return staged;
}

} // namespace mm6
