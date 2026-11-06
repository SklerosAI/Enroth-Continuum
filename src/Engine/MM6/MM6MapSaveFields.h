#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "MM6SaveContainer.h"

namespace mm6 {

struct SavedMapDelta {
    std::string name;
    std::vector<std::uint8_t> bytes;

    bool operator==(const SavedMapDelta &) const = default;
};

struct MapSaveFields {
    std::vector<SavedMapDelta> deltas;

    bool operator==(const MapSaveFields &) const = default;
};

// These are opaque MM6 DDM/DLV archive entries or decoded wire bytes. The
// world adapter must validate each against its corresponding base map before
// replacing live state; this codec only validates names, counts and lengths.
SaveSection encodeMapSection(const MapSaveFields &fields);
MapSaveFields decodeMapSection(const SaveContainer &container);
void loadMapFieldsBeforeMutation(std::span<const std::uint8_t> bytes, MapSaveFields *destination);

} // namespace mm6
