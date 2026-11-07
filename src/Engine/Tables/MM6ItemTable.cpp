#include "ItemTable.h"

#include "Engine/MM6/MM6StandardBonusMap.h"

#include <array>
#include <cctype>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/ResourceManager.h"

#include "Library/Lod/LodReader.h"
#include "Library/LodFormats/LodFormats.h"

#include "Library/Serialization/Serialization.h"
#include "Library/Logger/Logger.h"

#include "Utility/MapAccess.h"
#include "Utility/Exception.h"
#include "Utility/Memory/Blob.h"
#include "Utility/String/Ascii.h"
#include "Utility/String/Split.h"
#include "Utility/String/Transformations.h"

void ItemTable::InitializeMm6(ResourceManager *resourceManager) {
    MM_INFO("Loading MM6 item enchantments.");
    constexpr auto &attributes = mm6::kStandardBonusAttributes;

    Blob standardBlob = resourceManager->eventsData("stditems.txt");
    std::vector<std::string_view> standardLines = split(standardBlob.str()).by("\r\n");
    for (size_t i = 0; i < attributes.size(); i++) {
        std::array<std::string_view, 11> cells = split(standardLines[i + 4]).by('\t');
        auto &enchantment = standardEnchantments[attributes[i]];
        enchantment.attributeName = unquote(cells[0]);
        enchantment.itemSuffix = unquote(cells[1]);
        for (ItemType type : enchantment.chanceByItemType.indices())
            enchantment.chanceByItemType[type] = fromString<int>(cells[2 + std::to_underlying(type) - std::to_underlying(ITEM_TYPE_ARMOUR)]);
    }
    standardEnchantmentChanceSumByItemType.fill(0);
    for (Attribute attribute : attributes)
        for (ItemType type : standardEnchantments[attribute].chanceByItemType.indices())
            standardEnchantmentChanceSumByItemType[type] += standardEnchantments[attribute].chanceByItemType[type];

    for (ItemTreasureLevel level : standardEnchantmentRangeByTreasureLevel.indices()) {
        std::array<std::string_view, 4> cells = split(standardLines[22 + std::to_underlying(level)]).by('\t');
        standardEnchantmentRangeByTreasureLevel[level] = Segment(fromString<int>(cells[2]), fromString<int>(cells[3]));
    }

    Blob specialBlob = resourceManager->eventsData("spcitems.txt");
    std::vector<std::string_view> specialLines = split(specialBlob.str()).by("\r\n");
    for (int id = 1; id <= 59; id++) {
        std::array<std::string_view, 17> cells = split(specialLines[id + 3]).by('\t');
        auto &enchantment = specialEnchantments[static_cast<ItemEnchantment>(id)];
        enchantment.description = unquote(cells[0]);
        enchantment.itemSuffixOrPrefix = unquote(cells[1]);
        for (ItemType type : enchantment.chanceByItemType.indices())
            enchantment.chanceByItemType[type] = fromString<int>(cells[2 + std::to_underlying(type) - std::to_underlying(ITEM_TYPE_SINGLE_HANDED)]);
        std::string_view value = trim(cells[14]);
        if (!value.empty() && (value[0] == 'x' || value[0] == 'X')) {
            value.remove_prefix(1);
            enchantment.valueMul = fromString<int>(trim(value));
        } else {
            enchantment.valueAdd = fromString<int>(value);
        }
        enchantment.enchantmentLevel = !cells[15].empty() ? std::tolower(cells[15][0]) - 'a' : 0;
    }

    static const std::map<std::string, ItemType, ascii::NoCaseLess> itemTypes = {
        {"weapon", ITEM_TYPE_SINGLE_HANDED}, {"weapon2", ITEM_TYPE_TWO_HANDED},
        {"weapon1or2", ITEM_TYPE_SINGLE_HANDED}, {"missile", ITEM_TYPE_BOW},
        {"bow", ITEM_TYPE_BOW}, {"armor", ITEM_TYPE_ARMOUR}, {"shield", ITEM_TYPE_SHIELD},
        {"helm", ITEM_TYPE_HELMET}, {"belt", ITEM_TYPE_BELT}, {"cloak", ITEM_TYPE_CLOAK},
        {"gauntlets", ITEM_TYPE_GAUNTLETS}, {"boots", ITEM_TYPE_BOOTS}, {"ring", ITEM_TYPE_RING},
        {"amulet", ITEM_TYPE_AMULET}, {"weaponw", ITEM_TYPE_WAND}, {"herb", ITEM_TYPE_REAGENT},
        {"reagent", ITEM_TYPE_REAGENT}, {"bottle", ITEM_TYPE_POTION}, {"sscroll", ITEM_TYPE_SPELL_SCROLL},
        {"book", ITEM_TYPE_BOOK}, {"mscroll", ITEM_TYPE_MESSAGE_SCROLL}, {"gold", ITEM_TYPE_GOLD},
        {"gem", ITEM_TYPE_GEM}
    };
    static const std::map<std::string, Skill, ascii::NoCaseLess> itemSkills = {
        {"staff", SKILL_STAFF}, {"sword", SKILL_SWORD}, {"dagger", SKILL_DAGGER},
        {"axe", SKILL_AXE}, {"spear", SKILL_SPEAR}, {"bow", SKILL_BOW},
        {"mace", SKILL_MACE}, {"blaster", SKILL_BLASTER}, {"shield", SKILL_SHIELD},
        {"leather", SKILL_LEATHER}, {"chain", SKILL_CHAIN}, {"plate", SKILL_PLATE},
        {"club", SKILL_CLUB}
    };

    MM_INFO("Loading MM6 item templates.");
    Blob itemsBlob = resourceManager->eventsData("items.txt");
    for (std::string_view line : split(itemsBlob.str()).by("\r\n").drop(3).skip("")) {
        std::array<std::string_view, 16> cells = split(line).by('\t');
        int id = 0;
        if (!tryDeserialize(trim(cells[0]), &id) || cells[1].empty() ||
            id < std::to_underlying(ITEM_FIRST_VALID) || id > std::to_underlying(ITEM_LAST_VALID))
            continue;
        ItemData &item = items[static_cast<ItemId>(id)];
        item.iconName = unquote(cells[1]);
        item.name = unquote(cells[2]);
        item.baseValue = fromString<int>(cells[3]);
        item.type = valueOr(itemTypes, cells[4], ITEM_TYPE_MISC);
        item.skill = valueOr(itemSkills, cells[5], SKILL_MISC);
        std::string_view damage = cells[6];
        size_t d = damage.find_first_of("dD");
        if (d != std::string_view::npos) {
            item.damageDice = fromString<int>(damage.substr(0, d));
            item.damageRoll = fromString<int>(damage.substr(d + 1));
        } else if (!damage.empty() && std::isdigit(static_cast<unsigned char>(damage[0]))) {
            item.damageDice = fromString<int>(damage);
            item.damageRoll = 1;
        }
        item.damageMod = fromString<int>(cells[7]);
        item.rarity = ascii::noCaseEquals(cells[8], "artifact") ? RARITY_ARTIFACT
                    : ascii::noCaseEquals(cells[8], "relic") ? RARITY_RELIC : RARITY_COMMON;
        item.identifyAndRepairDifficulty = fromString<int>(cells[9]);
        item.unidentifiedName = unquote(cells[10]);
        item.spriteId = static_cast<SpriteId>(fromString<int>(cells[11]));
        item.paperdollAnchorOffset.x = fromString<int>(cells[13]);
        item.paperdollAnchorOffset.y = fromString<int>(cells[14]);
        item.description = unquote(cells[15]);
        if (item.type == ITEM_TYPE_REAGENT) {
            item.reagentPower = item.damageDice;
            item.damageDice = item.damageRoll = item.damageMod = 0;
        }
    }

    MM_INFO("Loading MM6 random item weights.");
    Blob randomBlob = resourceManager->eventsData("rnditems.txt");
    std::vector<std::string_view> randomLines = split(randomBlob.str()).by("\r\n");
    for (int id = 1; id <= 399; id++) {
        std::array<std::string_view, 8> cells = split(randomLines[id + 3]).by('\t');
        if (fromString<int>(cells[0]) != id)
            throw Exception("Unexpected MM6 random item id {} at row {}", cells[0], id);
        ItemData &item = items[static_cast<ItemId>(id)];
        for (ItemTreasureLevel level : item.uChanceByTreasureLvl.indices())
            item.uChanceByTreasureLvl[level] = fromString<int>(cells[1 + std::to_underlying(level)]);
    }
    itemChanceSumByTreasureLevel.fill(0);
    for (ItemTreasureLevel level : itemChanceSumByTreasureLevel.indices())
        for (ItemId id : items.indices())
            itemChanceSumByTreasureLevel[level] += items[id].uChanceByTreasureLvl[level];

    for (int row = 0; row < 3; row++) {
        std::array<std::string_view, 8> cells = split(randomLines[407 + row]).by('\t');
        auto *chances = row == 0 ? &standardEnchantmentChanceForEquipment
                      : row == 1 ? &specialEnchantmentChanceForEquipment : &specialEnchantmentChanceForWeapons;
        for (ItemTreasureLevel level : chances->indices())
            (*chances)[level] = fromString<int>(cells[1 + std::to_underlying(level)]);
    }

    Item::PopulateSpecialBonusMap();
    Item::PopulateArtifactBonusMap();
    LodReader icons(dfs->read("data/icons.lod"));
    Blob combinations = lod::decodeMaybeCompressed(icons.read("useitems.txt"));
    mm6PotionCombinations = mm6::PotionCombinations::parse(combinations.str());
    LoadItemSizes();
}
