#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Snapshots/CompositeSnapshots.h"

#include "MM6EventRuntimeSaveFields.h"
#include "MM6SaveContainer.h"

struct DecorationTable;

namespace mm6 {

// Together with the required IDCR native BLV anchor, one record owns every
// field of one ordered live LevelDecoration. Only the three fields that can
// differ after source reconstruction and event-variable preparation are stored.
struct IndoorDecorationRuntimeRecord {
    std::uint16_t descriptorId = 0;
    std::uint16_t flags = 0;
    std::int16_t eventVarId = 0;

    bool operator==(const IndoorDecorationRuntimeRecord &) const = default;
};

struct IndoorDecorationRuntimeFields {
    std::string mapName;
    std::vector<IndoorDecorationRuntimeRecord> records;

    bool operator==(const IndoorDecorationRuntimeFields &) const = default;
};

IndoorDecorationRuntimeFields captureIndoorDecorationRuntime(
    std::string_view mapName, const IndoorLocation_MM7 &base,
    const EventRuntimeMapEntry &events, const DecorationTable &table,
    std::span<const LevelDecoration> live, GameTitle title);
SaveSection encodeIndoorDecorationRuntimeSection(
    const IndoorDecorationRuntimeFields &fields, GameTitle title);
IndoorDecorationRuntimeFields decodeIndoorDecorationRuntimeSection(
    const SaveContainer &container, GameTitle title);
void requireIndoorDecorationRuntimeSourceAgreement(
    const IndoorDecorationRuntimeFields &fields, std::string_view mapName,
    const IndoorLocation_MM7 &base, const EventRuntimeMapEntry &events,
    const DecorationTable &table, GameTitle title);

// Reconstruct every source field and overlay the saved mutable owner. This is
// detached; the caller must prove the actual OnMapReload boundary separately.
std::vector<LevelDecoration> stageIndoorDecorationRuntime(
    const IndoorDecorationRuntimeFields &fields, const IndoorLocation_MM7 &base,
    const EventRuntimeMapEntry &events, const DecorationTable &table,
    GameTitle title);

// After real OnMapReload, apply on a copy and replace only after every slot
// has passed the immutable BLV identity checks. No live publication occurs in
// this helper; tests may supply a detached post-event vector.
void applyIndoorDecorationRuntimeAfterMapReload(
    const IndoorDecorationRuntimeFields &fields, const IndoorLocation_MM7 &base,
    const EventRuntimeMapEntry &events, const DecorationTable &table,
    std::vector<LevelDecoration> *afterEvent, GameTitle title);

} // namespace mm6
