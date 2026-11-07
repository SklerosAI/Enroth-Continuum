#pragma once

#include <stdexcept>

#include "Engine/GameTitle.h"
#include "Engine/Party.h"

#include "MM6IndoorFaceOverrides.h"

namespace mm6 {

// The exact nineteen-section v2 manifest has neither BEAC nor IFOV. A caller
// capturing live state must supply the observed indoor-face difference, even
// when it is empty, and reject state that the manifest cannot round-trip.
inline void requireRawV2NoBeacons(const Party &party, GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 raw v2 admission requires MM6 title");
    for (const Character &character : party.pCharacters)
        for (const auto &beacon : character.vBeacons)
            if (beacon)
                throw std::runtime_error("MM6 raw v2 cannot represent an active beacon");
}

inline void requireRawV2RepresentableState(const Party &party,
                                           const IndoorFaceOverrides &indoorFaces,
                                           GameTitle title) {
    requireRawV2NoBeacons(party, title);
    if (!indoorFaces.maps.empty())
        throw std::runtime_error("MM6 raw v2 cannot represent indoor face overrides");
}

} // namespace mm6
