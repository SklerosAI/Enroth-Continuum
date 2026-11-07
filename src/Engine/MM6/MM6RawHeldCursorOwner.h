#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Item.h"

#include "MM6ItemRecord28.h"
#include "MM6RawHeldItemSaveFields.h"

namespace mm6 {

// Detached MM6 cursor authority. The raw record remains present when its item
// number is zero, because the original clear retains bytes 25..27.
struct RawHeldCursorOwner {
    GameTitle title = GameTitle::MM6;
    bool ready = false;
    ItemRecord28 record;

    bool operator==(const RawHeldCursorOwner &) const = default;
};

inline bool validRawHeldCursorRecord(const ItemRecord28 &record) {
    const std::uint32_t id = record.itemNumberRaw();
    if (id)
        return id <= 580;
    return std::all_of(record.begin(), record.begin() + 25,
                       [](std::uint8_t byte) { return byte == 0; });
}

inline bool validRawHeldCursorOwner(const RawHeldCursorOwner &owner, GameTitle title) {
    return title == GameTitle::MM6 && owner.title == title && owner.ready &&
           validRawHeldCursorRecord(owner.record);
}

inline std::optional<RawHeldCursorOwner> freshRawHeldCursorOwner(GameTitle title) {
    if (title != GameTitle::MM6)
        return std::nullopt;
    return RawHeldCursorOwner{title, true, {}};
}

// Only an empty cursor can receive a record in this isolated seam. Relocating
// an occupied cursor to an inventory or ground owner needs a larger transaction.
inline std::optional<RawHeldCursorOwner> stageRawHeldCursorSet(
    const RawHeldCursorOwner &owner, const ItemRecord28 &incoming, GameTitle title) {
    if (!validRawHeldCursorOwner(owner, title) || owner.record.itemNumberRaw() ||
        incoming.itemNumberRaw() < 1 || incoming.itemNumberRaw() > 580)
        return std::nullopt;
    RawHeldCursorOwner staged = owner;
    staged.record = incoming; // Original setter copies all 28 bytes.
    return staged;
}

inline std::optional<RawHeldCursorOwner> stageRawHeldCursorClear(
    const RawHeldCursorOwner &owner, GameTitle title) {
    if (!validRawHeldCursorOwner(owner, title))
        return std::nullopt;
    RawHeldCursorOwner staged = owner;
    std::fill(staged.record.begin(), staged.record.begin() + 25, 0);
    return staged;
}

/**
 * Mirrors a live cursor pickup onto a ready raw owner. The original copies all 28 bytes of the incoming record.
 *
 * @param owner                         Raw cursor owner, left as is when it isn't ready or still holds a record.
 * @param item                          Item now on the live cursor. An item without a raw record leaves the owner
 *                                      as is, and the save-time cursor check then refuses the disagreement.
 * @param title                         Must be `GameTitle::MM6`.
 */
inline void syncRawHeldCursorSet(RawHeldCursorOwner *owner, const Item &item, GameTitle title) {
    if (!item.mm6RawRecord)
        return;
    if (auto staged = stageRawHeldCursorSet(*owner, *item.mm6RawRecord, title))
        *owner = *staged;
}

/**
 * Mirrors a live cursor clear onto a ready raw owner.
 *
 * @param owner                         Raw cursor owner, left as is when it isn't ready.
 * @param title                         Must be `GameTitle::MM6`.
 */
inline void syncRawHeldCursorClear(RawHeldCursorOwner *owner, GameTitle title) {
    if (auto staged = stageRawHeldCursorClear(*owner, title))
        *owner = *staged;
}

inline std::optional<RawHeldCursorOwner> stageRawHeldCursorFromFields(
    const RawHeldItemSaveFields &fields, GameTitle title) {
    if (title != GameTitle::MM6 || !validRawHeldCursorRecord(fields.record) ||
        fields.present != (fields.record.itemNumberRaw() != 0))
        return std::nullopt;
    return RawHeldCursorOwner{title, true, fields.record};
}

inline std::optional<RawHeldItemSaveFields> captureRawHeldCursor(
    const RawHeldCursorOwner &owner, GameTitle title) {
    if (!validRawHeldCursorOwner(owner, title))
        return std::nullopt;
    return RawHeldItemSaveFields{owner.record.itemNumberRaw() != 0, owner.record};
}

} // namespace mm6
