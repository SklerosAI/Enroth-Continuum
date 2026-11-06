#pragma once

#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>

#include "MM6KnownSaveFields.h"

#include "Utility/Memory/Blob.h"

namespace mm6 {

// The caller supplies MM6 base entries by basename from a trusted, title-
// specific content source. No filesystem or MM7 save path is chosen here.
using MapBaseReader = std::function<Blob(std::string_view)>;

// Decode every known EC6 section, check all 67 expected names, and validate
// every delta against its corresponding base before returning candidate state.
// The expected names must come from the locally verified private F3 manifest.
KnownSaveFields decodeKnownSaveWithMapBases(std::span<const std::uint8_t> bytes,
                                            std::span<const std::string> expectedNames,
                                            const MapBaseReader &readBase);

// Assign only after the entire known-save and map pass succeeds. This is not
// a live SaveLoad transaction and does not enable the save menu.
void loadKnownSaveWithMapBasesBeforeMutation(std::span<const std::uint8_t> bytes,
                                             std::span<const std::string> expectedNames,
                                             const MapBaseReader &readBase,
                                             KnownSaveFields *destination);

} // namespace mm6
