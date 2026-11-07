#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "MM6V5SaveFields.h"
#include "MM6VisitedOutdoorRuntimeOwner.h"

namespace mm6 {

// Detached outdoor branch of EC6 v6. The active map retains every v5 owner;
// VIST holds prior outdoor maps. No production writer or publisher consumes it.
struct V6OutdoorSaveFields {
    V5SaveFields active;
    std::vector<VisitedOutdoorRuntimeOwner> visited;

    [[nodiscard]] bool canWriteCheckpoint() const { return false; }
};

SaveSection encodeVisitedOutdoorSection(
    std::span<const VisitedOutdoorRuntimeOwner> visited, GameTitle title);
std::vector<VisitedOutdoorRuntimeOwner> decodeVisitedOutdoorSection(
    const SaveContainer &container, GameTitle title);

std::vector<std::uint8_t> encodeV6OutdoorSave(
    const V6OutdoorSaveFields &fields, GameTitle title);
V6OutdoorSaveFields decodeV6OutdoorSave(
    std::span<const std::uint8_t> bytes, GameTitle title);

} // namespace mm6
