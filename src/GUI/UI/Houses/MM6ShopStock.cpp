#include "MM6ShopStock.h"

#include <optional>
#include <stdexcept>
#include <vector>

#include "Core/Time/Duration.h"

#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/MM6/MM6Executable.h"
#include "Engine/MM6/MM6ItemGenerator.h"

namespace mm6 {

static ShopStockRow readShopStockRow(const Executable &exe, std::uint32_t address, int choices) {
    ShopStockRow result;
    result.treasureLevel = exe.i16(address);
    for (int i = 1; i <= choices; i++)
        result.choices.push_back(exe.i16(address + 2 * i));
    return result;
}

ShopStockRules decodeShopStockRules(const Executable &exe) {
    ShopStockRules result;
    for (int type = 0; type < 4; type++)
        result.visibleSlots[type] = exe.u8(0x4C45F1 + type); // A byte per house type from 0, weapon shops are type 1.
    for (int bank = 0; bank < 2; bank++) {
        const std::uint32_t offset = 540 * bank; // The second bank's tables follow the first one's.
        for (int houseId = 1; houseId <= 47; houseId++) {
            std::vector<ShopStockRow> &rows = result.banks[bank][houseId].rows;
            if (houseId < 15) {
                rows.push_back(readShopStockRow(exe, 0x4C433C + offset + 10 * (houseId + 15), 4));
            } else if (houseId < 29) {
                rows.push_back(readShopStockRow(exe, 0x4C433C + offset + 10 * (2 * houseId), 4));
                rows.push_back(readShopStockRow(exe, 0x4C433C + offset + 10 * (2 * houseId + 1), 4));
            } else if (houseId < 42) {
                rows.push_back(readShopStockRow(exe, 0x4C4546 + offset + 2 * houseId, 0));
            } else {
                rows.push_back(readShopStockRow(exe, 0x4C459C + offset + 14 * (houseId - 42), 6));
            }
        }
    }
    for (size_t i = 0; i < result.guildBooks.size(); i++)
        result.guildBooks[i] = exe.i16(0x4C48B0 + 2 * i);
    return result;
}

const ShopStockRules &shopStockRules() {
    static const ShopStockRules rules = decodeShopStockRules(executable());
    return rules;
}

int shopVisibleSlots(const ShopStockRules &rules, HouseType type) {
    switch (type) {
        case HOUSE_TYPE_WEAPON_SHOP: return rules.visibleSlots[0];
        case HOUSE_TYPE_ARMOR_SHOP: return rules.visibleSlots[1];
        case HOUSE_TYPE_MAGIC_SHOP: return rules.visibleSlots[2];
        case HOUSE_TYPE_GENERAL_STORE: return rules.visibleSlots[3];
        default: throw std::runtime_error("MM6 shop stock needs a shop house type");
    }
}

void generateShopBank(ShopStockFields *stock, const ShopStockRules &rules, ShopStockBank bank, int houseId,
                      HouseType type, const ShopRandom &random, const ShopItemGenerator &generator) {
    if (!stock || houseId < 1 || houseId > 47)
        throw std::runtime_error("MM6 shop bank generation needs a stock owner and a shop house");
    const ShopStockHouse &house = rules.banks[bank == ShopStockBank::First ? 0 : 1][houseId];
    const int slots = shopVisibleSlots(rules, type);
    for (int slot = 0; slot < slots; ++slot) {
        ItemRecord28 &record = stock->records[shopStockRecordIndex(bank, houseId, slot, GameTitle::MM6)];
        int treasureLevel = house.rows[0].treasureLevel;
        int kind = 0;
        if (houseId < 15) {
            kind = house.rows[0].choices[random(4)];
        } else if (houseId < 29) {
            const ShopStockRow &row = house.rows[slot > 3 ? 1 : 0];
            treasureLevel = row.treasureLevel;
            kind = row.choices[random(4)];
        } else if (houseId < 42) {
            kind = 22;
        } else if (const int item = house.rows[0].choices[random(6)]) {
            record = {};
            record.setWord(0, item);
        } else {
            kind = random(2) ? 37 : 39;
        }
        if (kind)
            generator(treasureLevel, kind, &record);
        record.setWord(20, 1); // Identified.
    }
}

void generateShopBankWithTitleRng(ShopStockFields *stock, const ShopStockRules &rules, ShopStockBank bank, int houseId,
                                  HouseType type, ChestTitleState *rng, const ItemTable &table, ItemKindRoll kindRoll) {
    if (!rng || !rng->ready)
        throw std::runtime_error("MM6 shop stock needs a restored gameplay LCG");
    const ShopRandom random = [rng](int hi) { return static_cast<int>(nextChestRandom15(rng->lcgState) % hi); };
    const ShopItemGenerator generator = [rng, &table, kindRoll](int treasureLevel, int itemKind, ItemRecord28 *record) {
        const std::optional<ChestFinalizedItem> fields = generateItem(GameTitle::MM6, table, static_cast<ItemTreasureLevel>(treasureLevel),
                                                                      static_cast<RandomItemType>(itemKind), kindRoll, rng->generatedArtifactFlags,
                                                                      rng->lcgState);
        if (!fields)
            throw std::runtime_error("MM6 shop stock request is outside the random item generator's tables");
        rng->lcgState = fields->nextRngState;
        rng->generatedArtifactFlags = fields->generatedArtifactFlags;
        *record = {};
        record->setWord(0, fields->itemId);
        record->setWord(4, fields->standardAttributeIndex);
        record->setWord(8, fields->standardStrength);
        record->setWord(12, fields->specialEnchantmentId);
        record->setWord(16, fields->numCharges);
        record->setWord(20, fields->flags);
    };
    generateShopBank(stock, rules, bank, houseId, type, random, generator);
}

void generateGuildStock(ShopStockFields *stock, const ShopStockRules &rules, int houseId, HouseType type, const ShopRandom &random) {
    if (!stock || houseId < 119 || houseId > 140 || type < HOUSE_TYPE_FIRE_GUILD || type > HOUSE_TYPE_MIRRORED_PATH_GUILD)
        throw std::runtime_error("MM6 guild stock generation needs a stock owner and a spell guild");
    const int books = rules.guildBooks[houseId - 119];
    for (int slot = 0; slot < 12; ++slot) {
        int school = std::to_underlying(type) - std::to_underlying(HOUSE_TYPE_FIRE_GUILD);
        if (type == HOUSE_TYPE_ELEMENTAL_GUILD) {
            school = random(4);
        } else if (type == HOUSE_TYPE_SELF_GUILD) {
            school = 4 + random(3);
        } else if (type == HOUSE_TYPE_MIRRORED_PATH_GUILD) {
            school = 7 + random(2);
        }
        ItemRecord28 &record = stock->records[guildStockRecordIndex(houseId, slot, GameTitle::MM6)];
        record.setWord(0, 300 + 11 * school + random(books)); // Each school has 11 books from item 300 on.
        record.setWord(20, 1); // Identified.
    }
}

void generateGuildStockWithTitleRng(ShopStockFields *stock, const ShopStockRules &rules, int houseId, HouseType type,
                                    ChestTitleState *rng) {
    if (!rng || !rng->ready)
        throw std::runtime_error("MM6 guild stock needs a restored gameplay LCG");
    generateGuildStock(stock, rules, houseId, type, [rng](int hi) { return static_cast<int>(nextChestRandom15(rng->lcgState) % hi); });
}

std::uint64_t shopRefreshTicks(int days) {
    return Duration::fromDays(days).ticks();
}

} // namespace mm6
