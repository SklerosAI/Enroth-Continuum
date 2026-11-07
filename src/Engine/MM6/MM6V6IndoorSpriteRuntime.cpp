#include "MM6V6IndoorSpriteRuntime.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

#include "Engine/Snapshots/EntitySnapshots.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"

#include "MM6V6IndoorSaveFields.h"

namespace mm6 {
namespace {

constexpr std::size_t NATIVE_WIDTH = 100;
constexpr std::size_t MAX_SPRITES = 4096;

void requireSourceShape(std::string_view mapName,
                        const IndoorNativeSlotAnchor &source, GameTitle title) {
    if (title != GameTitle::MM6 || mapName != source.mapName ||
        !mapName.ends_with(".blv") || source.count > MAX_SPRITES ||
        source.records.size() != source.count * NATIVE_WIDTH)
        throw std::runtime_error("MM6 ISRT lacks an exact ISCR indoor source");
}

std::array<std::uint8_t, NATIVE_WIDTH> syntheticAppendImage(
    const SpriteObject &runtime) {
    SpriteObject_MM7 image{};
    snapshot(runtime, &image);
    std::array<std::uint8_t, NATIVE_WIDTH> result{};
    std::memcpy(result.data(), &image, 36);
    if (runtime.containing_item.mm6RawRecord)
        std::copy(runtime.containing_item.mm6RawRecord->begin(),
                  runtime.containing_item.mm6RawRecord->end(),
                  result.begin() + 36);
    else
        std::memcpy(result.data() + 36, &image.containing_item, 28);
    std::memcpy(result.data() + 64,
        reinterpret_cast<const std::uint8_t *>(&image) + 72, 36);
    return result;
}

} // namespace

SpriteRuntimeSaveFields captureIndoorSpriteRuntime(
    std::string_view mapName, const IndoorNativeSlotAnchor &source,
    std::span<const SpriteObject> live, GameTitle title) {
    requireSourceShape(mapName, source, title);
    if (live.size() < source.count || live.size() > MAX_SPRITES)
        throw std::runtime_error("MM6 ISRT live full-list lacks its ISCR prefix");
    SpriteRuntimeSaveFields fields;
    fields.mapName = mapName;
    fields.nativePrefixCount = source.count;
    fields.records.reserve(live.size());
    for (std::size_t index = 0; index < live.size(); ++index) {
        SpriteRuntimeRecord record;
        if (index < source.count)
            std::copy_n(source.records.begin() + index * NATIVE_WIDTH,
                        NATIVE_WIDTH, record.nativeRecord.begin());
        else
            record.nativeRecord = syntheticAppendImage(live[index]);
        record.runtime = live[index];
        fields.records.push_back(std::move(record));
    }
    requireIndoorSpriteRuntimeSourceAgreement(fields, mapName, source, title);
    return fields;
}

void requireIndoorSpriteRuntimeSourceAgreement(
    const SpriteRuntimeSaveFields &fields, std::string_view mapName,
    const IndoorNativeSlotAnchor &source, GameTitle title) {
    requireSourceShape(mapName, source, title);
    if (fields.mapName != mapName ||
        fields.nativePrefixCount != source.count ||
        fields.records.size() < source.count ||
        fields.records.size() > MAX_SPRITES)
        throw std::runtime_error("MM6 ISRT map or slot count differs from ISCR");
    for (std::size_t index = 0; index < fields.records.size(); ++index) {
        std::array<std::uint8_t, NATIVE_WIDTH> expected{};
        if (index < source.count)
            std::copy_n(source.records.begin() + index * NATIVE_WIDTH,
                        NATIVE_WIDTH, expected.begin());
        else
            expected = syntheticAppendImage(fields.records[index].runtime);
        if (fields.records[index].nativeRecord != expected)
            throw std::runtime_error("MM6 ISRT native or appended image differs at slot " +
                                     std::to_string(index));
    }
    // The shared exact record codec checks fractional geometry, every runtime
    // field and the item image/raw provenance against the native record.
    (void) encodeSpriteRuntimeSectionAs(
        fields, SpriteRuntimeSectionKind::IndoorV6, title);
}

std::vector<SpriteObject> stageIndoorSpriteRuntime(
    const SpriteRuntimeSaveFields &fields, std::string_view mapName,
    const IndoorNativeSlotAnchor &source, GameTitle title) {
    requireIndoorSpriteRuntimeSourceAgreement(fields, mapName, source, title);
    std::vector<SpriteObject> result;
    result.reserve(fields.records.size());
    for (const auto &record : fields.records)
        result.push_back(record.runtime);
    return result;
}

void applyIndoorSpriteRuntimeAfterMapReload(
    const SpriteRuntimeSaveFields &fields, std::string_view mapName,
    const IndoorNativeSlotAnchor &source, std::vector<SpriteObject> *afterEvent,
    GameTitle title) {
    if (!afterEvent)
        throw std::runtime_error("MM6 ISRT lacks a replay destination");
    auto staged = stageIndoorSpriteRuntime(fields, mapName, source, title);
    if (afterEvent->size() < source.count)
        throw std::runtime_error("MM6 ISRT post-event list lacks native prefix");
    for (std::size_t index = 0; index < source.count; ++index) {
        const auto native = projectMM6GroundItemRecord(
            std::span(source.records).subspan(index * NATIVE_WIDTH, NATIVE_WIDTH));
        if (native.objectDescId == 0)
            continue; // The loader may reuse this source-free slot.
        if (std::to_underlying((*afterEvent)[index].spriteId) != native.spriteId ||
            (*afterEvent)[index].uObjectDescID != native.objectDescId ||
            (*afterEvent)[index].initialPosition != native.initialPosition.toFloat())
            throw std::runtime_error("MM6 ISRT post-event occupied native slot differs");
    }
    afterEvent->swap(staged);
}

} // namespace mm6
