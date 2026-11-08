#include "MM6ShopPrices.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

#include "Engine/Data/ItemData.h"
#include "Engine/Objects/Character.h"
#include "Engine/Objects/NPCEnums.h"
#include "Engine/Party.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/NPCTable.h"

static bool hasHiredProfession(NpcProfession profession) {
    for (int i = 0; i < pNPCStats->uNumNewNPCs; ++i)
        if (pNPCStats->pNPCData[i].profession == profession && (pNPCStats->pNPCData[i].flags & NPC_HIRED))
            return true;
    return pParty->pHirelings[0].profession == profession || pParty->pHirelings[1].profession == profession;
}

/**
 * @param base                          Full price of the service.
 * @param merchant                      Character's merchant percentage.
 * @return                              The price lowered by the merchant percentage, but not below a third of the full
 *                                      price, and not below 1.
 */
static int discountedServicePrice(int base, int merchant) {
    return std::max(1, std::max((100 - merchant) * base / 100, base / 3));
}

int mm6::merchantPercent(CombinedSkillValue merchant, int hirelingPoints) {
    const int points = merchant.level() + hirelingPoints;
    if (points == 0)
        return 0;
    const int multiplier = merchant.mastery() == MASTERY_MASTER ? 5 : merchant.mastery() == MASTERY_EXPERT ? 4 : 3;
    return multiplier * points + 7;
}

int mm6::hirelingMerchantPoints() {
    int result = 0;
    if (hasHiredProfession(NPC_PROFESSION_TRADER))
        result += 4;
    if (hasHiredProfession(NPC_PROFESSION_MERCHANT))
        result += 6;
    if (hasHiredProfession(NPC_PROFESSION_GYPSY))
        result += 3;
    if (hasHiredProfession(NPC_PROFESSION_DIPLOMAT)) // Negotiator in MM6.
        result += 4;
    if (hasHiredProfession(NPC_PROFESSION_DUPER))
        result += 8;
    return result;
}

int mm6::merchantPercent(const Character &character) {
    return merchantPercent(character.getSkillValue(SKILL_MERCHANT), hirelingMerchantPoints());
}

int mm6::itemValue(const ItemRecord28 &record, const ItemTable &table) {
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

int mm6::itemBuyPrice(int value, float multiplier, int merchant) {
    const int full = static_cast<int>(value * static_cast<double>(multiplier));
    return std::max(1, std::max((100 - merchant) * full / 100, value));
}

int mm6::itemSellPrice(int value, float multiplier, int merchant) {
    const int price = merchant * value / 100 + static_cast<int>(value / (static_cast<double>(multiplier) + 2.0));
    return std::max(1, std::min(price, value));
}

int mm6::itemIdentifyPrice(float multiplier, int merchant) {
    return discountedServicePrice(static_cast<int>(static_cast<double>(multiplier) * 50.0), merchant);
}

int mm6::itemRepairPrice(int value, float multiplier, int merchant) {
    return discountedServicePrice(static_cast<int>(value / (6.0 - static_cast<double>(multiplier))), merchant);
}

int mm6::itemSalePayment(int value, float multiplier, int merchant, bool broken, bool generalStore) {
    int result = broken ? 1 : itemSellPrice(value, multiplier, merchant);
    if (generalStore)
        result /= 2;
    return std::max(1, result);
}

bool mm6::shopTradesItem(int houseId, ItemId item, const ItemData &data) {
    const int id = std::to_underlying(item);
    if ((id >= 430 && id < 436) || id > 445)
        return false;
    // The original table loader stores an unknown or missing type as the one-handed weapon code.
    const int type = data.type == ITEM_TYPE_MISC ? std::to_underlying(ITEM_TYPE_SINGLE_HANDED) : std::to_underlying(data.type);
    if (houseId < 15)
        return type >= std::to_underlying(ITEM_TYPE_SINGLE_HANDED) && type <= std::to_underlying(ITEM_TYPE_BOW);
    if (houseId < 29)
        return type >= std::to_underlying(ITEM_TYPE_ARMOUR) && type <= std::to_underlying(ITEM_TYPE_BOOTS);
    if (houseId < 42)
        return data.skill == SKILL_MISC;
    return houseId < 48;
}
