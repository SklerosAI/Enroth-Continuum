#include "MM6DurableSaveFields.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace mm6 {
namespace {

constexpr std::array<char, 4> QEST_TAG = {'Q', 'E', 'S', 'T'};
constexpr std::array<char, 4> EFFT_TAG = {'E', 'F', 'F', 'T'};
constexpr std::array<char, 4> ITEM_TAG = {'I', 'T', 'E', 'M'};
constexpr std::uint8_t VERSION = 1;
constexpr size_t QEST_SIZE = 1 + 64 + 26 + 4 * 64 + 4 * 64;
constexpr size_t EFFT_SIZE = 1 + 4 * 19 * 8;
constexpr size_t ITEM_RECORD_SIZE = 49;
constexpr size_t MAX_ITEM_SIZE = 2 + 4 * (1 + 126 * ITEM_RECORD_SIZE);

class Reader {
 public:
    explicit Reader(std::span<const std::uint8_t> bytes) : _bytes(bytes) {}

    std::uint8_t u8() {
        if (_position == _bytes.size())
            throw std::runtime_error("truncated MM6 durable save field");
        return _bytes[_position++];
    }

    std::uint32_t u32() {
        std::uint32_t result = 0;
        for (int shift = 0; shift < 32; shift += 8)
            result |= std::uint32_t(u8()) << shift;
        return result;
    }

    std::uint64_t u64() {
        std::uint64_t result = 0;
        for (int shift = 0; shift < 64; shift += 8)
            result |= std::uint64_t(u8()) << shift;
        return result;
    }

    template <size_t N>
    void bytes(std::array<std::uint8_t, N> *destination) {
        for (std::uint8_t &byte : *destination)
            byte = u8();
    }

    void finish() const {
        if (_position != _bytes.size())
            throw std::runtime_error("trailing MM6 durable save field bytes");
    }

 private:
    std::span<const std::uint8_t> _bytes;
    size_t _position = 0;
};

void put8(std::vector<std::uint8_t> *bytes, std::uint8_t value) { bytes->push_back(value); }

void put32(std::vector<std::uint8_t> *bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        put8(bytes, value >> shift);
}

void put64(std::vector<std::uint8_t> *bytes, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8)
        put8(bytes, value >> shift);
}

template <size_t N>
void putBytes(std::vector<std::uint8_t> *bytes, const std::array<std::uint8_t, N> &source) {
    bytes->insert(bytes->end(), source.begin(), source.end());
}

void validateItem(const DurableItem &item) {
    if (item.sourceIndex >= 126 || item.itemId <= 0 || item.itemId > 65535 ||
        item.standardEnchantment < -1 || item.standardEnchantment > 255 ||
        item.specialEnchantment > 65535 || item.charges < 0 || item.maxCharges < 0 ||
        item.expirationTicks < 0)
        throw std::runtime_error("invalid MM6 durable item field");
    if (item.zone == 1) {
        if (item.x >= 14 || item.y >= 9 || item.equipmentSlot != 0)
            throw std::runtime_error("invalid MM6 inventory grid location");
    } else if (item.zone == 2) {
        if (item.equipmentSlot < 1 || item.equipmentSlot > 16 || item.x || item.y)
            throw std::runtime_error("invalid MM6 equipment location");
    } else {
        throw std::runtime_error("invalid MM6 item location kind");
    }
}

void validate(const DurableSaveFields &fields) {
    for (const auto &conditions : fields.effects.conditionTicks)
        for (std::int64_t ticks : conditions)
            if (ticks < 0)
                throw std::runtime_error("negative MM6 condition timestamp");

    for (const auto &inventory : fields.inventory.characters) {
        if (inventory.size() > 126)
            throw std::runtime_error("MM6 character inventory exceeds count limit");
        std::array<bool, 126> recordSeen{};
        std::array<bool, 17> slotSeen{};
        for (const DurableItem &item : inventory) {
            validateItem(item);
            if (std::exchange(recordSeen[item.sourceIndex], true))
                throw std::runtime_error("duplicate MM6 inventory record index");
            if (item.zone == 2 && std::exchange(slotSeen[item.equipmentSlot], true))
                throw std::runtime_error("duplicate MM6 equipment slot");
        }
    }
}

std::vector<std::uint8_t> encodeBits(const DurableBits &bits) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(QEST_SIZE);
    put8(&bytes, VERSION);
    putBytes(&bytes, bits.quest);
    putBytes(&bytes, bits.autonotes);
    for (const auto &awards : bits.awards)
        putBytes(&bytes, awards);
    for (const auto &events : bits.characterEvents)
        putBytes(&bytes, events);
    return bytes;
}

DurableBits decodeBits(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != QEST_SIZE)
        throw std::runtime_error("invalid MM6 QEST field size");
    Reader reader(bytes);
    if (reader.u8() != VERSION)
        throw std::runtime_error("unsupported MM6 QEST field version");
    DurableBits result;
    reader.bytes(&result.quest);
    reader.bytes(&result.autonotes);
    for (auto &awards : result.awards)
        reader.bytes(&awards);
    for (auto &events : result.characterEvents)
        reader.bytes(&events);
    reader.finish();
    return result;
}

std::vector<std::uint8_t> encodeEffects(const DurableEffects &effects) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(EFFT_SIZE);
    put8(&bytes, VERSION);
    for (const auto &conditions : effects.conditionTicks)
        for (std::int64_t ticks : conditions)
            put64(&bytes, std::bit_cast<std::uint64_t>(ticks));
    return bytes;
}

DurableEffects decodeEffects(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != EFFT_SIZE)
        throw std::runtime_error("invalid MM6 EFFT field size");
    Reader reader(bytes);
    if (reader.u8() != VERSION)
        throw std::runtime_error("unsupported MM6 EFFT field version");
    DurableEffects result;
    for (auto &conditions : result.conditionTicks)
        for (std::int64_t &ticks : conditions)
            ticks = std::bit_cast<std::int64_t>(reader.u64());
    reader.finish();
    return result;
}

std::vector<std::uint8_t> encodeItems(const DurableInventory &inventory) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(MAX_ITEM_SIZE);
    put8(&bytes, VERSION);
    put8(&bytes, 4);
    for (const auto &character : inventory.characters) {
        put8(&bytes, character.size());
        for (const DurableItem &item : character) {
            put8(&bytes, item.sourceIndex);
            put8(&bytes, item.zone);
            put8(&bytes, item.x);
            put8(&bytes, item.y);
            put8(&bytes, item.equipmentSlot);
            put32(&bytes, std::bit_cast<std::uint32_t>(item.itemId));
            put32(&bytes, std::bit_cast<std::uint32_t>(item.potionPower));
            put32(&bytes, std::bit_cast<std::uint32_t>(item.goldAmount));
            put32(&bytes, std::bit_cast<std::uint32_t>(item.standardEnchantment));
            put32(&bytes, std::bit_cast<std::uint32_t>(item.standardStrength));
            put32(&bytes, item.specialEnchantment);
            put32(&bytes, std::bit_cast<std::uint32_t>(item.charges));
            put32(&bytes, std::bit_cast<std::uint32_t>(item.maxCharges));
            put32(&bytes, item.flags);
            put64(&bytes, std::bit_cast<std::uint64_t>(item.expirationTicks));
        }
    }
    return bytes;
}

DurableInventory decodeItems(std::span<const std::uint8_t> bytes) {
    if (bytes.size() > MAX_ITEM_SIZE)
        throw std::runtime_error("MM6 ITEM field exceeds size limit");
    Reader reader(bytes);
    if (reader.u8() != VERSION || reader.u8() != 4)
        throw std::runtime_error("unsupported MM6 ITEM field version or party size");
    DurableInventory result;
    for (auto &inventory : result.characters) {
        const size_t count = reader.u8();
        if (count > 126)
            throw std::runtime_error("MM6 inventory count exceeds limit");
        inventory.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            DurableItem item;
            item.sourceIndex = reader.u8();
            item.zone = reader.u8();
            item.x = reader.u8();
            item.y = reader.u8();
            item.equipmentSlot = reader.u8();
            item.itemId = std::bit_cast<std::int32_t>(reader.u32());
            item.potionPower = std::bit_cast<std::int32_t>(reader.u32());
            item.goldAmount = std::bit_cast<std::int32_t>(reader.u32());
            item.standardEnchantment = std::bit_cast<std::int32_t>(reader.u32());
            item.standardStrength = std::bit_cast<std::int32_t>(reader.u32());
            item.specialEnchantment = reader.u32();
            item.charges = std::bit_cast<std::int32_t>(reader.u32());
            item.maxCharges = std::bit_cast<std::int32_t>(reader.u32());
            item.flags = reader.u32();
            item.expirationTicks = std::bit_cast<std::int64_t>(reader.u64());
            inventory.push_back(item);
        }
    }
    reader.finish();
    return result;
}

} // namespace

std::array<SaveSection, 3> encodeDurableSections(const DurableSaveFields &fields) {
    validate(fields);
    return {SaveSection{QEST_TAG, true, encodeBits(fields.bits)},
            SaveSection{EFFT_TAG, true, encodeEffects(fields.effects)},
            SaveSection{ITEM_TAG, true, encodeItems(fields.inventory)}};
}

DurableSaveFields decodeDurableSections(const SaveContainer &container) {
    const SaveSection *quest = nullptr;
    const SaveSection *effects = nullptr;
    const SaveSection *items = nullptr;
    for (const SaveSection &section : container.sections) {
        if (section.tag == QEST_TAG) quest = &section;
        if (section.tag == EFFT_TAG) effects = &section;
        if (section.tag == ITEM_TAG) items = &section;
    }
    if (!quest || !effects || !items || !quest->required || !effects->required || !items->required)
        throw std::runtime_error("MM6 durable save fields are missing or optional");

    DurableSaveFields result;
    result.bits = decodeBits(quest->payload);
    result.effects = decodeEffects(effects->payload);
    result.inventory = decodeItems(items->payload);
    validate(result);
    return result;
}

void loadDurableFieldsBeforeMutation(std::span<const std::uint8_t> bytes, DurableSaveFields *destination) {
    if (!destination)
        throw std::runtime_error("null MM6 durable save destination");
    DurableSaveFields staged = decodeDurableSections(decodeSaveContainer(bytes));
    *destination = std::move(staged);
}

} // namespace mm6
