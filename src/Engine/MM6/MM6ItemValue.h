#pragma once

#include <cstdint>
#include <stdexcept>

#include "Engine/MM6/MM6ItemRecord28.h"
#include "Engine/Tables/ItemTable.h"

namespace mm6 {

/**
 * MM6 item value. A standard bonus adds 100 gold per point of strength. A special bonus whose value is above 10 adds
 * that much gold, otherwise it multiplies the base value.
 *
 * @param record                        Original 28-byte item record.
 * @param table                         MM6 item table.
 * @return                              Item value in gold.
 */
inline int itemValue(const ItemRecord28 &record, const ItemTable &table) {
    const std::uint32_t id = record.itemNumberRaw();
    if (id < 1 || id > 580)
        throw std::runtime_error("MM6 item value needs a valid item number");
    const int base = table.items[static_cast<ItemId>(id)].baseValue;
    if (record.bonusRaw())
        return base + 100 * static_cast<std::int32_t>(record.bonusStrengthRaw());
    const std::uint32_t special = record.secondBonusRaw();
    if (!special)
        return base;
    if (special > 59)
        throw std::runtime_error("MM6 item value needs a valid special bonus");
    const SpecialEnchantmentData &data = table.specialEnchantments[static_cast<ItemEnchantment>(special)];
    const int stored = data.valueAdd ? data.valueAdd : data.valueMul;
    return stored > 10 ? base + stored : base * stored;
}

}  // namespace mm6
