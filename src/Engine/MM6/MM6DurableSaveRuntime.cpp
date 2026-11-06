#include "MM6DurableSaveFields.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

#include "Engine/Engine.h"
#include "Engine/Party.h"
#include "Engine/Tables/ItemTable.h"

namespace mm6 {
namespace {

void requireMM6(GameTitle title, bool requireEngine) {
    if (title != GameTitle::MM6 || (requireEngine && !engine) ||
        (engine && engine->gameTitle() != GameTitle::MM6))
        throw std::runtime_error("MM6 durable save fields cannot apply to another title");
}

template <typename Bitset, size_t N>
void captureBits(const Bitset &source, std::array<std::uint8_t, N> *target) {
    for (size_t bit = 0; bit < N * 8; ++bit)
        if (source.test(static_cast<typename Bitset::key_type>(bit + 1)))
            (*target)[bit / 8] |= std::uint8_t(1) << (bit % 8);
}

template <typename Bitset, size_t N>
void stageBits(const std::array<std::uint8_t, N> &source, Bitset *target) {
    target->reset();
    for (size_t bit = 0; bit < N * 8; ++bit)
        if ((source[bit / 8] >> (bit % 8)) & 1)
            target->set(static_cast<typename Bitset::key_type>(bit + 1));
}

void applyBitsAndConditions(const DurableSaveFields &fields, Party *staged) {
    stageBits(fields.bits.quest, &staged->_questBits);
    stageBits(fields.bits.autonotes, &staged->_autonoteBits);
    for (size_t index = 0; index < staged->pCharacters.size(); ++index) {
        Character &character = staged->pCharacters[index];
        stageBits(fields.bits.awards[index], &character._achievedAwardsBits);
        stageBits(fields.bits.characterEvents[index], &character._characterEventBits);
        for (size_t condition = 0; condition < 19; ++condition)
            character.conditions.set(static_cast<Condition>(condition),
                                     Time::fromTicks(fields.effects.conditionTicks[index][condition]));
    }
}

} // namespace

DurableSaveFields captureDurableFields(const Party &party, GameTitle title) {
    requireMM6(title, true);
    DurableSaveFields fields;
    captureBits(party._questBits, &fields.bits.quest);
    captureBits(party._autonoteBits, &fields.bits.autonotes);

    for (size_t index = 0; index < party.pCharacters.size(); ++index) {
        const Character &character = party.pCharacters[index];
        captureBits(character._achievedAwardsBits, &fields.bits.awards[index]);
        captureBits(character._characterEventBits, &fields.bits.characterEvents[index]);
        for (size_t condition = 0; condition < 19; ++condition)
            fields.effects.conditionTicks[index][condition] =
                character.conditions.get(static_cast<Condition>(condition)).ticks();

        for (InventoryConstEntry entry : character.inventory.entries()) {
            if (entry.index() >= 126)
                throw std::runtime_error("MM6 inventory record index exceeds save limit");
            DurableItem item;
            item.sourceIndex = entry.index();
            if (entry.zone() == INVENTORY_ZONE_GRID) {
                item.zone = 1;
                item.x = entry.geometry().x;
                item.y = entry.geometry().y;
            } else if (entry.zone() == INVENTORY_ZONE_EQUIPMENT) {
                item.zone = 2;
                item.equipmentSlot = std::to_underlying(entry.slot());
            } else {
                throw std::runtime_error("unsupported MM6 character inventory zone");
            }
            item.itemId = std::to_underlying(entry->itemId);
            item.potionPower = entry->potionPower;
            item.goldAmount = entry->goldAmount;
            item.standardEnchantment = entry->standardEnchantment
                ? std::to_underlying(*entry->standardEnchantment) : -1;
            item.standardStrength = entry->standardEnchantmentStrength;
            item.specialEnchantment = std::to_underlying(entry->specialEnchantment);
            item.charges = entry->numCharges;
            item.maxCharges = entry->maxCharges;
            item.flags = static_cast<std::uint32_t>(entry->flags);
            item.expirationTicks = entry->enchantmentExpirationTime.ticks();
            fields.inventory.characters[index].push_back(item);
        }
    }
    (void) encodeDurableSections(fields);
    return fields;
}

Party stageDurablePartyWithoutItems(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title) {
    requireMM6(title, false);
    DurableSaveFields fields;
    loadDurableFieldsBeforeMutation(bytes, &fields);
    for (const auto &inventory : fields.inventory.characters)
        if (!inventory.empty())
            throw std::runtime_error("MM6 item restoration needs a title-specific inventory adapter");

    Party staged = live;
    applyBitsAndConditions(fields, &staged);
    return staged;
}

Party stageDurableParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title) {
    requireMM6(title, false);
    DurableSaveFields fields;
    loadDurableFieldsBeforeMutation(bytes, &fields);
    const bool hasItems = std::any_of(fields.inventory.characters.begin(), fields.inventory.characters.end(),
                                      [](const auto &inventory) { return !inventory.empty(); });
    if (hasItems && !pItemTable)
        throw std::runtime_error("MM6 item table must be loaded before inventory staging");

    // All mutations below are on a copy. Any invalid ID, changed icon footprint,
    // overlap or record-index conflict leaves the caller's party untouched.
    Party staged = live;
    applyBitsAndConditions(fields, &staged);
    for (size_t index = 0; index < staged.pCharacters.size(); ++index) {
        CharacterInventory &inventory = staged.pCharacters[index].inventory;
        inventory.clear();
        for (const DurableItem &source : fields.inventory.characters[index]) {
            if (source.itemId < 1 || source.itemId > 580 ||
                source.standardEnchantment > std::to_underlying(ATTRIBUTE_LAST_ENCHANTABLE) ||
                source.specialEnchantment > 59)
                throw std::runtime_error("MM6 saved item has an unsupported title-specific ID");

            const ItemId itemId = static_cast<ItemId>(source.itemId);
            if (pItemTable->items[itemId].iconName.empty())
                throw std::runtime_error("MM6 saved item has no loaded template icon");

            Item item;
            item.itemId = itemId;
            item.potionPower = source.potionPower;
            item.goldAmount = source.goldAmount;
            if (source.standardEnchantment >= 0)
                item.standardEnchantment = static_cast<Attribute>(source.standardEnchantment);
            item.standardEnchantmentStrength = source.standardStrength;
            item.specialEnchantment = static_cast<ItemEnchantment>(source.specialEnchantment);
            item.numCharges = source.charges;
            item.maxCharges = source.maxCharges;
            item.flags = ItemFlags(source.flags);
            item.enchantmentExpirationTime = Time::fromTicks(source.expirationTicks);

            InventoryEntry restored = source.zone == 1
                ? inventory.tryRestoreAt(source.sourceIndex, Pointi(source.x, source.y), item)
                : inventory.tryRestoreAt(source.sourceIndex, static_cast<ItemSlot>(source.equipmentSlot), item);
            if (!restored || restored.index() != source.sourceIndex)
                throw std::runtime_error("MM6 saved item footprint or indexed placement is invalid");
        }
    }
    return staged;
}

} // namespace mm6
