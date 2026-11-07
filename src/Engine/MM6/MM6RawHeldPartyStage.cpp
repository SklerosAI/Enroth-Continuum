#include "MM6RawHeldPartyStage.h"

#include <stdexcept>

#include "MM6BoundedItemProjection.h"

namespace mm6 {

Party stageRawHeldParty(const Party &baseline, const RawHeldCursorOwner &cursor,
                        const ItemTable &itemTable, GameTitle title) {
    if (!validRawHeldCursorOwner(cursor, title))
        throw std::runtime_error("MM6 raw held Party stage needs a ready cursor owner");

    Party staged = baseline;
    staged.pPickedItem.Reset();
    if (!cursor.record.itemNumberRaw())
        return staged;

    auto projection = stageBoundedItem(cursor.record, itemTable, title);
    if (!projection)
        throw std::runtime_error("MM6 raw held Party stage cannot project item semantics");
    staged.pPickedItem = boundedItem(*projection);
    setBoundedItem(*projection, staged.pPickedItem);
    const auto replay = captureBoundedItem(*projection, itemTable, title);
    if (!replay || *replay != cursor.record)
        throw std::runtime_error("MM6 raw held Party stage cannot replay physical record");
    return staged;
}

} // namespace mm6
