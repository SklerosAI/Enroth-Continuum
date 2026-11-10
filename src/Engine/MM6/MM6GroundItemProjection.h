#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "Engine/GameTitle.h"
#include "Engine/Tables/ItemTable.h"

#include "MM6BoundedItemProjection.h"

namespace mm6 {

// The MM6 map wire owns a 100-byte sprite record with an exact 28-byte item
// at byte 36. This detached view never reconstructs it through Item_MM7.
class GroundItemProjection {
 public:
    explicit GroundItemProjection(std::array<std::uint8_t, 100> sourceRecord)
        : _sourceRecord(sourceRecord) {}

    const std::array<std::uint8_t, 100> &sourceRecord() const { return _sourceRecord; }
    std::optional<BoundedItemProjection> item;

 private:
    std::array<std::uint8_t, 100> _sourceRecord;
};

inline ItemRecord28 groundItemRaw(const std::array<std::uint8_t, 100> &record) {
    ItemRecord28 item;
    std::copy_n(record.begin() + 36, item.size(), item.begin());
    return item;
}

inline std::optional<GroundItemProjection> stageGroundItemRecord(
    std::span<const std::uint8_t> bytes, const ItemTable &table, GameTitle title) {
    if (title != GameTitle::MM6 || bytes.size() != 100)
        return std::nullopt;

    std::array<std::uint8_t, 100> sourceRecord;
    std::copy(bytes.begin(), bytes.end(), sourceRecord.begin());
    GroundItemProjection result(sourceRecord);
    const ItemRecord28 raw = groundItemRaw(result.sourceRecord());
    const std::uint16_t objectDescId = std::uint16_t(bytes[2]) | (std::uint16_t(bytes[3]) << 8);
    if (!objectDescId || !raw.itemNumberRaw())
        return result; // Removed sprites and empty contained items keep opaque bytes.

    // Ground equipment location and potion-power unions have no proven live
    // MM6 interpretation. Reject them before creating a runtime Item view.
    if (raw.bodyLocationRaw() || raw.itemNumberRaw() > 580 ||
        table.items[static_cast<ItemId>(raw.itemNumberRaw())].type == ITEM_TYPE_POTION)
        return std::nullopt;
    result.item = stageBoundedItem(raw, table, title);
    if (!result.item || boundedItem(*result.item).mm6RawRecord != raw)
        return std::nullopt;
    return result;
}

// Only source-backed Item fields may change. The sprite header, placement,
// untouched item bytes and sprite tail stay in the retained map record.
inline std::optional<std::array<std::uint8_t, 100>> captureGroundItemRecord(
    const GroundItemProjection &projection, const ItemTable &table, GameTitle title) {
    const auto source = stageGroundItemRecord(projection.sourceRecord(), table, title);
    if (!source || source->item.has_value() != projection.item.has_value())
        return std::nullopt;

    auto result = projection.sourceRecord();
    if (projection.item) {
        if (projection.item->index() != source->item->index())
            return std::nullopt;
        const auto item = captureBoundedItem(*projection.item, table, title);
        if (!item)
            return std::nullopt;
        std::copy(item->begin(), item->end(), result.begin() + 36);
    }
    return result;
}

} // namespace mm6
