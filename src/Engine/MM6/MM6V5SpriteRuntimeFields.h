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
    // ISRT only: the exact native ISCR prefix length. Remaining slots were
    // appended at runtime and carry canonical synthetic image anchors.
    std::uint32_t nativePrefixCount = 0;
};

enum class SpriteRuntimeSectionKind { OutdoorV5, IndoorV6 };

// Share the exact per-sprite image/float/item codec without changing the
// published v5 SPRT map or tag contract. Indoor v6 uses a distinct ISRT tag.
SaveSection encodeSpriteRuntimeSectionAs(
    const SpriteRuntimeSaveFields &fields, SpriteRuntimeSectionKind kind,
    GameTitle title);
SpriteRuntimeSaveFields decodeSpriteRuntimeSectionAs(
    const SaveContainer &container, SpriteRuntimeSectionKind kind,
    GameTitle title);

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

/**
 * @param fields                        Saved SPRT list.
 * @param rebuilt                       Sprites the native map load rebuilt, after `arrangeSpriteObjects`.
 * @param mapName                       File name of the loaded map.
 * @param nativeDelta                   Live native DDM bytes of the loaded map.
 * @param title                         Must be MM6.
 * @return                              Copy of the saved runtime sprites. Throws unless every saved record matches the
 *                                      native DDM, and both the rebuilt and the saved sprite of each slot keep its
 *                                      native sprite, object descriptor and initial position.
 */
std::vector<SpriteObject> stageSpriteRuntimeReplay(
    const SpriteRuntimeSaveFields &fields, std::span<const SpriteObject> rebuilt,
    std::string_view mapName, std::span<const std::uint8_t> nativeDelta, GameTitle title);

} // namespace mm6
