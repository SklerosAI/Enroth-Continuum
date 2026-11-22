#include "Engine/MM6/MM6ItemGenerator.h"

#include <stdexcept>

namespace mm6 {

ItemRecord28 generatedItemRecord(const ChestFinalizedItem &fields) {
    ItemRecord28 result;
    result.setWord(0, fields.itemId);
    result.setWord(4, fields.standardAttributeIndex);
    result.setWord(8, fields.standardStrength);
    result.setWord(12, fields.specialEnchantmentId);
    result.setWord(16, fields.numCharges);
    result.setWord(20, fields.flags);
    return result;
}

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
    item.mm6RawRecord = generatedItemRecord(fields);
    return item;
}

StagedTitleItem stageTitleItem(const ChestTitleState &state, const ItemTable &table, ItemTreasureLevel treasureLevel,
                               RandomItemType requestedKind, ItemKindRoll kindRoll,
                               const std::array<std::uint8_t, 30> *collectedArtifactFlags) {
    if (!state.ready)
        throw std::runtime_error("MM6 random item needs a restored gameplay LCG");
    const auto fields = generateItem(GameTitle::MM6, table, treasureLevel, requestedKind, kindRoll,
                                     state.generatedArtifactFlags, state.lcgState, collectedArtifactFlags);
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
