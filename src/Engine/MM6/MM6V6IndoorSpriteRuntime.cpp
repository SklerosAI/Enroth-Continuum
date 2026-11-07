#include "MM6V6IndoorSpriteRuntime.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>

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

} // namespace

SpriteRuntimeSaveFields captureIndoorSpriteRuntime(
    std::string_view mapName, const IndoorNativeSlotAnchor &source,
    std::span<const SpriteObject> live, GameTitle title) {
    requireSourceShape(mapName, source, title);
    if (live.size() != source.count)
        throw std::runtime_error("MM6 ISRT live full-list count differs from ISCR");
    SpriteRuntimeSaveFields fields;
    fields.mapName = mapName;
    fields.records.reserve(live.size());
    for (std::size_t index = 0; index < live.size(); ++index) {
        SpriteRuntimeRecord record;
        std::copy_n(source.records.begin() + index * NATIVE_WIDTH,
                    NATIVE_WIDTH, record.nativeRecord.begin());
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
    if (fields.mapName != mapName || fields.records.size() != source.count)
        throw std::runtime_error("MM6 ISRT map or slot count differs from ISCR");
    for (std::size_t index = 0; index < fields.records.size(); ++index)
        if (!std::equal(fields.records[index].nativeRecord.begin(),
                        fields.records[index].nativeRecord.end(),
                        source.records.begin() + index * NATIVE_WIDTH))
            throw std::runtime_error("MM6 ISRT native record differs at slot " +
                                     std::to_string(index));
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
    if (afterEvent->size() != staged.size())
        throw std::runtime_error("MM6 ISRT post-event slot count differs");
    for (std::size_t index = 0; index < staged.size(); ++index) {
        if ((*afterEvent)[index].spriteId != staged[index].spriteId ||
            (*afterEvent)[index].uObjectDescID != staged[index].uObjectDescID ||
            (*afterEvent)[index].initialPosition != staged[index].initialPosition)
            throw std::runtime_error("MM6 ISRT post-event native identity differs");
    }
    afterEvent->swap(staged);
}

} // namespace mm6
