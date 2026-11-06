#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "MM6ChestTitleState.h"
#include "MM6SaveContainer.h"

namespace mm6 {

// CHST v1 retains the MM6-only chest RNG and artifact ledger, plus the full
// decoded active DLV/DDM used as the source for later changed-state writes.
struct ChestSaveFields {
    std::uint32_t lcgState = 0;
    std::array<std::uint8_t, 30> generatedArtifactFlags = {};
    std::string activeDeltaName;
    std::vector<std::uint8_t> rawActiveDelta;

    bool operator==(const ChestSaveFields &) const = default;
};

ChestSaveFields captureChestSaveFields(const ChestTitleState &owner,
                                      std::string activeDeltaName,
                                      std::span<const std::uint8_t> rawActiveDelta,
                                      GameTitle title);
SaveSection encodeChestSection(const ChestSaveFields &fields);
ChestSaveFields decodeChestSection(const SaveContainer &container);

// Prepare both owners before either is assigned. No global owner is touched.
void restoreChestFieldsBeforeMutation(const ChestSaveFields &fields,
                                      ChestTitleState *owner,
                                      std::vector<std::uint8_t> *rawActiveDelta,
                                      GameTitle title);

} // namespace mm6
