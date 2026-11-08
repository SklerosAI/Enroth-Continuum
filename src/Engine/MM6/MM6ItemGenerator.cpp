#include "Engine/MM6/MM6ItemGenerator.h"

#include <stdexcept>

namespace mm6 {

Item generatedRuntimeItem(const ChestFinalizedItem &fields) {
    if (fields.itemId < 1 || fields.itemId > 429 ||
        (fields.standardAttributeIndex && !attributeForStandardBonus(fields.standardAttributeIndex)) ||
        fields.specialEnchantmentId < 0 || fields.specialEnchantmentId > 58 || fields.numCharges < 0)
        throw std::runtime_error("MM6 generated item fields are outside the generator's range");
    Item item;
    item.itemId = static_cast<ItemId>(fields.itemId);
    if (fields.standardAttributeIndex) {
        item.standardEnchantment = attributeForStandardBonus(fields.standardAttributeIndex);
        item.standardEnchantmentStrength = fields.standardStrength;
    }
    item.specialEnchantment = static_cast<ItemEnchantment>(fields.specialEnchantmentId);
    item.numCharges = fields.numCharges;
    item.flags = ItemFlags(fields.flags);
    return item;
}

StagedTitleItem stageTitleItem(const ChestTitleState &state, const ItemTable &table, ItemTreasureLevel treasureLevel,
                               RandomItemType requestedKind) {
    if (!state.ready)
        throw std::runtime_error("MM6 random item needs a restored gameplay LCG");
    const auto fields = generateItem(GameTitle::MM6, table, treasureLevel, requestedKind,
                                     state.generatedArtifactFlags, state.lcgState);
    if (!fields)
        throw std::runtime_error("MM6 random item request is outside the original generator's tables");

    StagedTitleItem result;
    result.item = generatedRuntimeItem(*fields);
    result.nextState = state;
    result.nextState.lcgState = fields->nextRngState;
    result.nextState.generatedArtifactFlags = fields->generatedArtifactFlags;
    return result;
}

} // namespace mm6
