#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string_view>

#include "Engine/GameTitle.h"
#include "Engine/MM6/MM6OutdoorBorderEligibility.h"
#include "Engine/MM6/MM6OutdoorBorderTravel.h"
#include "Engine/Party.h"

namespace mm6 {

enum class BorderPrecommitRefusal {
    WrongTitle,
    PartyState,
    NoCardinalRoute,
    CornerUnverified,
    MissingWorldAndSourceProof,
};

struct BorderPrecommitStage {
    std::optional<BorderTravelCandidate> destination;
    std::unique_ptr<Party> oldMapParty;
    BorderPrecommitRefusal refusal = BorderPrecommitRefusal::NoCardinalRoute;

    // A detached Party copy is not a complete old-map save. Source archive,
    // visited-map and active runtime owners still require a transaction.
    [[nodiscard]] bool canWriteCheckpoint() const { return false; }
};

// Stage only the old-map Party owner at the confirmation boundary. Original
// MM6's rejected/cancelled border path clamps both axes; use those same bounds
// on a copy so a future old-map reload does not start beyond the source edge.
// This function never changes live Party, disk, map cache or pending travel.
[[nodiscard]] inline BorderPrecommitStage stageBorderOldMapParty(
    GameTitle title, std::string_view sourceMap, const Party &party,
    const BorderTravelHirelings &hirelings) {
    if (title != GameTitle::MM6)
        return {.refusal = BorderPrecommitRefusal::WrongTitle};
    if (!borderOfferPartyFlagsPermit(title, party.uFlags) ||
        !borderConfirmFlyingStatePermits(title, party.bFlying) ||
        !std::isfinite(party.pos.z))
        return {.refusal = BorderPrecommitRefusal::PartyState};

    const BorderTravelAssessment route = assessOutdoorBorderTravel(
        title, sourceMap, party.pos.x, party.pos.y, hirelings);
    if (!route.candidate)
        return {.refusal = BorderPrecommitRefusal::NoCardinalRoute};
    if (route.candidate->crossedX && route.candidate->crossedY)
        return {.refusal = BorderPrecommitRefusal::CornerUnverified};

    auto oldMapParty = std::make_unique<Party>(party);
    oldMapParty->pos.x = std::clamp(oldMapParty->pos.x,
                                   -MM6_OUTDOOR_BORDER_EDGE, MM6_OUTDOOR_BORDER_EDGE);
    oldMapParty->pos.y = std::clamp(oldMapParty->pos.y,
                                   -MM6_OUTDOOR_BORDER_EDGE, MM6_OUTDOOR_BORDER_EDGE);
    return {.destination = route.candidate, .oldMapParty = std::move(oldMapParty),
            .refusal = BorderPrecommitRefusal::MissingWorldAndSourceProof};
}

} // namespace mm6
