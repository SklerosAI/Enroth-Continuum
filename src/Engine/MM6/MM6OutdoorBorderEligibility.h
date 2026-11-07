#pragma once

#include "Engine/GameTitle.h"
#include "Engine/PartyEnums.h"

namespace mm6 {

// The original outdoor edge caller checks these three Party bits after the
// geometric neighbor succeeds. This only models that caller's flag check;
// it does not authorize an offer or encode the full travel transaction.
[[nodiscard]] inline bool borderOfferPartyFlagsPermit(GameTitle title, PartyFlags flags) {
    if (title != GameTitle::MM6)
        return false;
    return !(flags & (PARTY_FLAG_WATER_DAMAGE | PARTY_FLAG_AIRBORNE | PARTY_FLAG_STANDING_ON_WATER));
}

// The original border-confirm branch separately refuses a nonzero flying
// state before it rechecks the neighbor. It does not repeat the flag check.
[[nodiscard]] inline bool borderConfirmFlyingStatePermits(GameTitle title, unsigned int flying) {
    return title == GameTitle::MM6 && flying == 0;
}

} // namespace mm6
