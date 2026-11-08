#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

#include "Engine/Data/HouseEnums.h"
#include "Engine/MM6/MM6ShopStockFields.h"
#include "Engine/MM6/MM6ShopStockIdentity.h"

class Blob;
struct ItemTable;

namespace mm6 {

struct ChestTitleState;

/**
 * Fills a cleared 28-byte record with a random item of the given kind and treasure level. Throws when it can't.
 */
using ShopItemGenerator = std::function<void(int treasureLevel, int itemKind, ItemRecord28 *record)>;

/**
 * Draws a random number in `[0, hi)`.
 */
using ShopRandom = std::function<int(int hi)>;

struct ShopStockRow {
    int treasureLevel = 0;
    std::vector<int> choices; // Item kinds, or for general stores item ids where 0 means a generated item.
};

/**
 * Stock generation table of one shop bank, from the private MM6 rules profile. Weapon, magic and general shops have
 * one row. Armor shops have two, the second one for slots 4 and up.
 */
struct ShopStockHouse {
    std::vector<ShopStockRow> rows;
};

struct ShopStockRules {
    std::array<int, 4> visibleSlots = {}; // Weapon, armor, magic and general shops.
    std::array<std::array<ShopStockHouse, 48>, 2> banks; // Indexed by bank, then by MM6 house id 1..47.
    std::array<int, 22> guildBooks = {}; // Books per school a spell guild offers, by MM6 house id from 119.
};

ShopStockRules decodeShopStockRules(const Blob &source);

/**
 * @return                              Shop stock rules loaded once from `rules/shop-stock.json` in the MM6 profile.
 */
const ShopStockRules &shopStockRules();

/**
 * @param type                          Shop's house type.
 * @return                              How many stock slots the shop shows and generates.
 */
int shopVisibleSlots(const ShopStockRules &rules, HouseType type);

/**
 * Regenerates the visible slots of one shop bank the way the original game does. Slots past the visible count keep
 * their bytes, and every generated slot is marked identified.
 *
 * @param stock                         Physical stock to write.
 * @param rules                         Shop stock rules.
 * @param bank                          Shop bank to regenerate.
 * @param houseId                       MM6 house id of the shop, 1..47.
 * @param type                          Shop's house type.
 * @param random                        Random numbers for the slot choices.
 * @param generator                     Random item generator.
 */
void generateShopBank(ShopStockFields *stock, const ShopStockRules &rules, ShopStockBank bank, int houseId,
                      HouseType type, const ShopRandom &random, const ShopItemGenerator &generator);

/**
 * `generateShopBank` with the slot choices and items drawn from the MM6 gameplay LCG, through the MM6 random item
 * generator, as the original game does.
 *
 * @param stock                         Physical stock to write.
 * @param rules                         Shop stock rules.
 * @param bank                          Shop bank to regenerate.
 * @param houseId                       MM6 house id of the shop, 1..47.
 * @param type                          Shop's house type.
 * @param rng                           Gameplay LCG and artifact flags, advanced by every draw.
 * @param table                         MM6 item table.
 */
void generateShopBankWithTitleRng(ShopStockFields *stock, const ShopStockRules &rules, ShopStockBank bank, int houseId,
                                  HouseType type, ChestTitleState *rng, const ItemTable &table);

/**
 * Regenerates the twelve spellbook slots of a spell guild the way the original game does. A guild of one school offers
 * its first books, a guild of several schools picks a school per slot first. Only the item id and the identified flag
 * of a slot are written, the other bytes stay.
 *
 * @param stock                         Physical stock to write.
 * @param rules                         Shop stock rules.
 * @param houseId                       MM6 house id of the guild, 119..140.
 * @param type                          Guild's house type.
 * @param random                        Random numbers for the school and book choices.
 */
void generateGuildStock(ShopStockFields *stock, const ShopStockRules &rules, int houseId, HouseType type, const ShopRandom &random);

/**
 * `generateGuildStock` with the choices drawn from the MM6 gameplay LCG.
 *
 * @param stock                         Physical stock to write.
 * @param rules                         Shop stock rules.
 * @param houseId                       MM6 house id of the guild, 119..140.
 * @param type                          Guild's house type.
 * @param rng                           Gameplay LCG, advanced by every draw.
 */
void generateGuildStockWithTitleRng(ShopStockFields *stock, const ShopStockRules &rules, int houseId, HouseType type,
                                    ChestTitleState *rng);

/**
 * @param days                          The shop's refresh interval in days, column 15 of 2DEvents.txt.
 * @return                              The interval in game ticks.
 */
std::uint64_t shopRefreshTicks(int days);

} // namespace mm6
