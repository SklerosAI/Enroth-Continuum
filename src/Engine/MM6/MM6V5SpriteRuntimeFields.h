#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Objects/SpriteObject.h"

#include "MM6SaveContainer.h"

namespace mm6 {

// Detached v5 runtime truth. Each ordered slot retains its exact 100-byte MM6
// DDM source record as an anchor; the complete SpriteObject state is separate.
// No native ground-item bytes are rewritten from fractional runtime values.
struct SpriteRuntimeRecord {
    std::array<std::uint8_t, 100> nativeRecord{};
    SpriteObject runtime;
};

struct SpriteRuntimeSaveFields {
    std::string mapName;
    std::vector<SpriteRuntimeRecord> records;
};

SpriteRuntimeSaveFields captureSpriteRuntimeFields(
    std::string_view mapName, std::span<const std::uint8_t> nativeDelta,
    std::span<const SpriteObject> live, GameTitle title);
SaveSection encodeSpriteRuntimeSection(const SpriteRuntimeSaveFields &fields,
                                       GameTitle title);
SpriteRuntimeSaveFields decodeSpriteRuntimeSection(const SaveContainer &container,
                                                   GameTitle title);
void requireSpriteRuntimeNativeAgreement(
    const SpriteRuntimeSaveFields &fields, std::string_view mapName,
    std::span<const std::uint8_t> nativeDelta, GameTitle title);

} // namespace mm6
