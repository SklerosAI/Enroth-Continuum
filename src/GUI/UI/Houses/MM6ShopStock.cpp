#include "MM6ShopStock.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include "Core/Time/Duration.h"

#include "Engine/MM6/MM6ChestTitleState.h"
#include "Engine/MM6/MM6ItemGenerator.h"
#include "Engine/Resources/EngineFileSystem.h"

#include "Library/Json/Json.h"

#include "Utility/Memory/Blob.h"

namespace mm6 {
namespace {

int boundedInt(const Json &value, int low, int high) {
    if (!value.is_number_integer())
        throw std::runtime_error("Invalid MM6 shop stock integer type");
    const int result = value.get<int>();
    if (result < low || result > high)
        throw std::runtime_error("MM6 shop stock integer outside verified bounds");
    return result;
}

size_t expectedRows(int houseId) {
    return houseId >= 15 && houseId < 29 ? 2 : 1;
}

size_t expectedChoices(int houseId) {
    if (houseId < 29)
        return 4;
    return houseId < 42 ? 0 : 6;
}

} // namespace

ShopStockRules decodeShopStockRules(const Blob &source) {
    if (source.size() > 65536)
        throw std::runtime_error("MM6 shop stock manifest is too large");
    const Json json = Json::parse(source.str());
    if (boundedInt(json.at("format"), 1, 1) != 1 || !json.at("visible").is_array() || json.at("visible").size() != 4 ||
        !json.at("banks").is_array() || json.at("banks").size() != 2)
        throw std::runtime_error("MM6 shop stock manifest has an unknown shape");

    ShopStockRules result;
    for (size_t i = 0; i < 4; ++i)
        result.visibleSlots[i] = boundedInt(json.at("visible")[i], 1, 12);
    for (size_t bank = 0; bank < 2; ++bank) {
        const Json &houses = json.at("banks")[bank];
        if (!houses.is_array() || houses.size() != 47)
            throw std::runtime_error("MM6 shop stock bank needs 47 shops");
        for (size_t i = 0; i < 47; ++i) {
            const Json &house = houses[i];
            const int houseId = boundedInt(house.at("house"), 1, 47);
            if (houseId != static_cast<int>(i) + 1 || !house.at("rows").is_array() || house.at("rows").size() != expectedRows(houseId))
                throw std::runtime_error("MM6 shop stock rows are missing or out of order");
            for (const Json &row : house.at("rows")) {
                if (!row.at("choices").is_array() || row.at("choices").size() != expectedChoices(houseId))
                    throw std::runtime_error("MM6 shop stock row has the wrong number of choices");
                ShopStockRow decoded;
                decoded.treasureLevel = boundedInt(row.at("level"), 1, 6);
                for (const Json &choice : row.at("choices"))
                    decoded.choices.push_back(boundedInt(choice, 0, 580));
                result.banks[bank][houseId].rows.push_back(std::move(decoded));
            }
        }
    }
    const Json &guilds = json.at("guilds");
    if (!guilds.is_array() || guilds.size() != result.guildBooks.size())
        throw std::runtime_error("MM6 shop stock manifest needs 22 spell guilds");
    for (size_t i = 0; i < guilds.size(); ++i) {
        if (boundedInt(guilds[i].at("house"), 119, 140) != 119 + static_cast<int>(i))
            throw std::runtime_error("MM6 spell guild rows are missing or out of order");
        result.guildBooks[i] = boundedInt(guilds[i].at("books"), 1, 11);
    }
    return result;
}

const ShopStockRules &shopStockRules() {
    static const ShopStockRules rules = decodeShopStockRules(ufs->read("rules/shop-stock.json"));
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
                                  HouseType type, ChestTitleState *rng, const ItemTable &table) {
    if (!rng || !rng->ready)
        throw std::runtime_error("MM6 shop stock needs a restored gameplay LCG");
    const ShopRandom random = [rng](int hi) { return static_cast<int>(nextChestRandom15(rng->lcgState) % hi); };
    const ShopItemGenerator generator = [rng, &table](int treasureLevel, int itemKind, ItemRecord28 *record) {
        const std::optional<ChestFinalizedItem> fields = generateItem(GameTitle::MM6, table, static_cast<ItemTreasureLevel>(treasureLevel),
                                                                      static_cast<RandomItemType>(itemKind), rng->generatedArtifactFlags, rng->lcgState);
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
