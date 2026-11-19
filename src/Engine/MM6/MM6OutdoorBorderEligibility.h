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

/**
 * The GrayFace patch's offer check, the original flag check plus the confirmation's flying check, so the crossing isn't
 * offered to a party flying close enough to the ground to clear the flags.
 *
 * @param title                         Game title, only MM6 is accepted.
 * @param flags                         Party flags.
 * @param flying                        Party flying state.
 * @return                              Whether the crossing may be offered.
 */
[[nodiscard]] inline bool borderOfferPartyStatePermits(GameTitle title, PartyFlags flags, unsigned int flying) {
    return borderOfferPartyFlagsPermit(title, flags) && borderConfirmFlyingStatePermits(title, flying);
}

} // namespace mm6
