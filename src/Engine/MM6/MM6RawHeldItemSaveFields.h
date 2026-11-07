#pragma once

#include <cstdint>
#include <span>

#include "Engine/GameTitle.h"

#include "MM6ItemRecord28.h"
#include "MM6SaveContainer.h"

namespace mm6 {

// A detached candidate for HELD v2. It preserves one exact MM6 cursor record,
// but does not attach a runtime cursor, infer item type, or enter KnownSaveFields.
struct RawHeldItemSaveFields {
    bool present = false;
    ItemRecord28 record;

    bool operator==(const RawHeldItemSaveFields &) const = default;
};

SaveSection encodeRawHeldItemSection(const RawHeldItemSaveFields &fields, GameTitle title);
RawHeldItemSaveFields decodeRawHeldItemSection(const SaveContainer &container, GameTitle title);
void loadRawHeldItemBeforeMutation(std::span<const std::uint8_t> bytes,
                                   RawHeldItemSaveFields *destination, GameTitle title);

} // namespace mm6
