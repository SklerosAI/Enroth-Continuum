#pragma once

#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "Engine/PartyPlacement.h"

#include "MM6OutdoorBorderTravel.h"
#include "MM6OutdoorReturnArrival.h"
#include "MM6LoadedMapSource.h"
#include "MM6V6OutdoorSaveFields.h"
#include "Engine/Snapshots/MM6OutdoorSnapshot.h"

namespace mm6 {

enum class VisitedOutdoorReturnRefusal {
    MissingArrival,
    UnprovedTerrainAndPublication,
};

// An owned, read-only return candidate. Native/runtime map state comes from
// VIST, while the global chest RNG/artifact ledger remains current from the
// active map. Neither a Party arrival nor failure-atomic world publication is
// implied by the saved VIST payload.
struct VisitedOutdoorReturnPlan {
    std::string activeMap;
    VisitedOutdoorRuntimeOwner visited;
    ChestSaveFields returnChest;
    std::optional<PartyPlacement> proposedArrival;
    VisitedOutdoorReturnRefusal refusal =
        VisitedOutdoorReturnRefusal::MissingArrival;

    [[nodiscard]] bool canPublish() const { return false; }
    [[nodiscard]] bool canWriteCheckpoint() const { return false; }
};

inline VisitedOutdoorReturnPlan stageVisitedOutdoorReturnPlan(
    const V6OutdoorSaveFields &saved, std::string_view activeMap,
    std::string_view targetMap, const Blob &targetBase,
    const DecorationTable &decorationTable,
    std::optional<PartyPlacement> arrival, GameTitle title) {
    if (title != GameTitle::MM6 || activeMap == targetMap ||
        saved.active.v4.v3.raw.metadata.header.mapFileName != activeMap ||
        saved.visited.size() != 1 || saved.visited.front().mapName != targetMap)
        throw std::runtime_error("MM6 visited return lacks exact map pair");
    (void) encodeV6OutdoorSave(saved, title);
    requireVisitedOutdoorRuntimeOwner(saved.visited.front(), targetMap,
                                      targetBase, decorationTable, title);

    ChestSaveFields returnChest = saved.active.v4.v3.raw.chest;
    returnChest.activeDeltaName = saved.visited.front().nativeDelta.name;
    returnChest.rawActiveDelta = saved.visited.front().nativeDelta.bytes;
    (void) encodeChestSection(returnChest);

    if (arrival && (!std::isfinite(arrival->pos.x) ||
                    !std::isfinite(arrival->pos.y) ||
                    !std::isfinite(arrival->pos.z) ||
                    std::abs(arrival->pos.x) > MM6_OUTDOOR_BORDER_EDGE ||
                    std::abs(arrival->pos.y) > MM6_OUTDOOR_BORDER_EDGE))
        throw std::runtime_error("MM6 visited return arrival is not in bounds");

    return {std::string(activeMap), saved.visited.front(),
            std::move(returnChest), arrival,
            arrival ? VisitedOutdoorReturnRefusal::UnprovedTerrainAndPublication :
                      VisitedOutdoorReturnRefusal::MissingArrival};
}

// Validate the active/visited owner and source archive before decoding terrain.
// This returns only a detached Party copy. No map or save publisher consumes it.
[[nodiscard]] inline OutdoorReturnArrival stageVisitedOutdoorReturnArrival(
    const V6OutdoorSaveFields &saved, std::string_view activeMap,
    std::string_view targetMap, const LoadedMapSource &source,
    const DecorationTable &decorationTable, const Party &party, GameTitle title) {
    const Blob targetBase = source.readBase(targetMap);
    (void) stageVisitedOutdoorReturnPlan(saved, activeMap, targetMap, targetBase,
                                        decorationTable, std::nullopt, title);
    auto base = std::make_unique<OutdoorLocation_MM7>();
    deserializeMM6Outdoor(targetBase, base.get());
    return stageDetachedOutdoorReturnArrival(title, activeMap, targetMap,
                                             *base, party);
}

} // namespace mm6
