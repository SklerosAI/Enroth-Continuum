#pragma once

#include "Engine/MM6/MM6ItemRecord28.h"
#include "Engine/Objects/CombinedSkillValue.h"
#include "Engine/Objects/ItemEnums.h"

class Character;
struct Item;
struct ItemData;
struct ItemTable;

namespace mm6 {

/**
 * MM6 merchant percentage. Unlike MM7, reputation plays no part and the result has no upper limit.
 *
 * @param merchant                      Character's own merchant skill.
 * @param hirelingPoints                Merchant points the party's hired NPCs add.
 * @return                              Percentage taken off buying and service prices and added to selling prices.
 *                                      0 when skill and hirelings add up to no points.
 */
int merchantPercent(CombinedSkillValue merchant, int hirelingPoints);

/**
 * @return                              Merchant points the party's hired NPCs add. Each merchant profession counts
 *                                      once, whether a hired NPC or a party hireling has it.
 */
int hirelingMerchantPoints();

/**
 * @param character                     Character who trades.
 * @return                              The character's merchant percentage, with the party's hirelings.
 */
int merchantPercent(const Character &character);

/**
 * MM6 item value. A standard bonus adds 100 gold per point of strength. A special bonus whose value is above 10 adds
 * that much gold, otherwise it multiplies the base value.
 *
 * @param record                        Original 28-byte item record.
 * @param table                         MM6 item table.
 * @return                              Item value in gold.
 */
int itemValue(const ItemRecord28 &record, const ItemTable &table);

/**
 * MM6 item value of a live item. Uses the item's original record when it has one, otherwise its bonus fields. A potion
 * without a record has no known value and throws.
 *
 * @param item                          Item to value.
 * @param table                         MM6 item table.
 * @return                              Item value in gold.
 */
int itemValue(const Item &item, const ItemTable &table);

/**
 * @param value                         Item value.
 * @param multiplier                    Shop's `Val` column in 2DEvents.txt.
 * @param merchant                      Buyer's merchant percentage.
 * @return                              Gold the shop asks, never below the item value.
 */
int itemBuyPrice(int value, float multiplier, int merchant);

/**
 * @param value                         Item value.
 * @param multiplier                    Shop's `Val` column in 2DEvents.txt.
 * @param merchant                      Seller's merchant percentage.
 * @return                              Gold the shop offers, between 1 and the item value.
 */
int itemSellPrice(int value, float multiplier, int merchant);

/**
 * @param multiplier                    Shop's `Val` column in 2DEvents.txt.
 * @param merchant                      Character's merchant percentage.
 * @return                              Gold the shop asks to identify an item, at least a third of the full price.
 */
int itemIdentifyPrice(float multiplier, int merchant);

/**
 * @param value                         Item value.
 * @param multiplier                    Shop's `Val` column in 2DEvents.txt.
 * @param merchant                      Character's merchant percentage.
 * @return                              Gold the shop asks to repair the item, at least a third of the full price.
 */
int itemRepairPrice(int value, float multiplier, int merchant);

/**
 * @param value                         Item value.
 * @param multiplier                    Shop's `Val` column in 2DEvents.txt.
 * @param merchant                      Seller's merchant percentage.
 * @param broken                        Whether the item is broken.
 * @param generalStore                  Whether the shop is a general store.
 * @return                              Gold the party gets for selling the item. A broken item fetches 1 gold, and a
 *                                      general store pays half.
 */
int itemSalePayment(int value, float multiplier, int merchant, bool broken, bool generalStore);

/**
 * Whether a shop buys, identifies and repairs an item. MM6 decides this by house id, not by shop type: weapon shops
 * take weapons, armor shops take armor, magic shops take items with no weapon or armor skill, and general stores
 * take anything. A few item ids are taken by no shop.
 *
 * @param houseId                       MM6 house id of the shop.
 * @param item                          Item id.
 * @param data                          The item's entry in the MM6 item table.
 * @return                              Whether the shop deals in the item.
 */
bool shopTradesItem(int houseId, ItemId item, const ItemData &data);

} // namespace mm6
