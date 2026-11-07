#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Decoration.h"

#include "MM6SaveContainer.h"

namespace mm6 {

// The inherited arrays are runtime state, not a projection of MM6's opaque
// 200-byte native DDM/DLV persistent section. A binding guards the ordinal
// assigned by RespawnGlobalDecorations/PrepareDecorations against map changes.
struct EventRuntimeDecorationBinding {
    std::uint32_t decorationIndex = 0;
    std::uint16_t descriptorId = 0;
    std::uint16_t cog = 0;
    std::uint8_t eventVarId = 0;
    std::array<std::uint32_t, 3> positionBits{};
    std::int32_t yawAngle = 0;
    std::uint16_t triggerRange = 0;

    bool operator==(const EventRuntimeDecorationBinding &) const = default;
};

struct EventRuntimeMapEntry {
    std::string mapName;
    std::uint32_t respawnCount = 0;
    std::uint32_t lastRespawnDay = 0;
    std::array<std::uint8_t, 75> mapVars{};
    std::array<std::uint8_t, 125> decorVars{};
    std::vector<EventRuntimeDecorationBinding> bindings;

    bool operator==(const EventRuntimeMapEntry &) const = default;
};

struct EventRuntimeSaveFields {
    // Sparse entries for visited maps; sorted by canonical map name.
    std::vector<EventRuntimeMapEntry> maps;

    bool operator==(const EventRuntimeSaveFields &) const = default;
};

// Computes only the first 124 interactive decorations without an explicit
// event ID, in source order. This reads but never changes the decorations.
std::vector<EventRuntimeDecorationBinding> captureEventRuntimeDecorationBindings(
    std::span<LevelDecoration> decorations, GameTitle title);

SaveSection encodeEventRuntimeSection(const EventRuntimeSaveFields &fields,
                                      GameTitle title);
EventRuntimeSaveFields decodeEventRuntimeSection(const SaveContainer &container,
                                                 GameTitle title);

// Caller supplies the already validated native DDM/DLV reset header and the
// rebuilt decoration list. Missing or stale entries refuse before mutation.
const EventRuntimeMapEntry &requireEventRuntimeMapEntry(
    const EventRuntimeSaveFields &fields, std::string_view mapName,
    std::uint32_t nativeRespawnCount, std::uint32_t nativeLastRespawnDay,
    std::span<LevelDecoration> decorations, GameTitle title);

} // namespace mm6
