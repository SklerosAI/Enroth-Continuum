#include "MM6HeldItemSaveFields.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mm6 {
namespace {

constexpr std::array<char, 4> TAG = {'H', 'E', 'L', 'D'};
constexpr std::uint8_t VERSION = 1;
constexpr size_t EMPTY_SIZE = 4;
constexpr size_t PRESENT_SIZE = EMPTY_SIZE + 10 * 4 + 8;

void put32(std::vector<std::uint8_t> *bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        bytes->push_back(value >> shift);
}

void put64(std::vector<std::uint8_t> *bytes, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8)
        bytes->push_back(value >> shift);
}

std::uint32_t get32(std::span<const std::uint8_t> bytes, size_t *offset) {
    std::uint32_t value = 0;
    for (int shift = 0; shift < 32; shift += 8)
        value |= std::uint32_t(bytes[(*offset)++]) << shift;
    return value;
}

std::uint64_t get64(std::span<const std::uint8_t> bytes, size_t *offset) {
    std::uint64_t value = 0;
    for (int shift = 0; shift < 64; shift += 8)
        value |= std::uint64_t(bytes[(*offset)++]) << shift;
    return value;
}

void validate(const HeldItemSaveFields &fields) {
    if (!fields.present) {
        if (fields != HeldItemSaveFields())
            throw std::runtime_error("noncanonical empty MM6 held item");
        return;
    }
    if (fields.itemId < 1 || fields.itemId > 580 ||
        fields.standardEnchantment < -1 || fields.standardEnchantment > 23 ||
        fields.specialEnchantment > 59 || fields.charges < 0 ||
        fields.maxCharges < 0 || fields.lichJarCharacterIndex != -1 ||
        fields.expirationTicks < 0)
        throw std::runtime_error("unsupported MM6 held item field");
}

} // namespace

SaveSection encodeHeldItemSection(const HeldItemSaveFields &fields) {
    validate(fields);
    std::vector<std::uint8_t> bytes;
    bytes.reserve(fields.present ? PRESENT_SIZE : EMPTY_SIZE);
    bytes.insert(bytes.end(), {VERSION, std::uint8_t(fields.present), 0, 0});
    if (fields.present) {
        put32(&bytes, std::bit_cast<std::uint32_t>(fields.itemId));
        put32(&bytes, std::bit_cast<std::uint32_t>(fields.potionPower));
        put32(&bytes, std::bit_cast<std::uint32_t>(fields.goldAmount));
        put32(&bytes, std::bit_cast<std::uint32_t>(fields.standardEnchantment));
        put32(&bytes, std::bit_cast<std::uint32_t>(fields.standardStrength));
        put32(&bytes, fields.specialEnchantment);
        put32(&bytes, std::bit_cast<std::uint32_t>(fields.charges));
        put32(&bytes, std::bit_cast<std::uint32_t>(fields.maxCharges));
        put32(&bytes, fields.flags);
        put32(&bytes, std::bit_cast<std::uint32_t>(fields.lichJarCharacterIndex));
        put64(&bytes, std::bit_cast<std::uint64_t>(fields.expirationTicks));
    }
    return {TAG, true, std::move(bytes)};
}

HeldItemSaveFields decodeHeldItemSection(const SaveContainer &container) {
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections) {
        if (candidate.tag == TAG) {
            if (section)
                throw std::runtime_error("duplicate MM6 HELD section");
            section = &candidate;
        }
    }
    if (!section || !section->required || section->payload.size() < EMPTY_SIZE)
        throw std::runtime_error("missing or invalid MM6 HELD section");
    std::span<const std::uint8_t> bytes = section->payload;
    if (bytes[0] != VERSION || bytes[1] > 1 || bytes[2] || bytes[3] ||
        bytes.size() != (bytes[1] ? PRESENT_SIZE : EMPTY_SIZE))
        throw std::runtime_error("unsupported MM6 HELD field version or shape");

    HeldItemSaveFields result;
    result.present = bytes[1] != 0;
    if (result.present) {
        size_t offset = EMPTY_SIZE;
        result.itemId = std::bit_cast<std::int32_t>(get32(bytes, &offset));
        result.potionPower = std::bit_cast<std::int32_t>(get32(bytes, &offset));
        result.goldAmount = std::bit_cast<std::int32_t>(get32(bytes, &offset));
        result.standardEnchantment = std::bit_cast<std::int32_t>(get32(bytes, &offset));
        result.standardStrength = std::bit_cast<std::int32_t>(get32(bytes, &offset));
        result.specialEnchantment = get32(bytes, &offset);
        result.charges = std::bit_cast<std::int32_t>(get32(bytes, &offset));
        result.maxCharges = std::bit_cast<std::int32_t>(get32(bytes, &offset));
        result.flags = get32(bytes, &offset);
        result.lichJarCharacterIndex = std::bit_cast<std::int32_t>(get32(bytes, &offset));
        result.expirationTicks = std::bit_cast<std::int64_t>(get64(bytes, &offset));
    }
    validate(result);
    return result;
}

void loadHeldItemBeforeMutation(std::span<const std::uint8_t> bytes,
                                HeldItemSaveFields *destination) {
    HeldItemSaveFields staged = decodeHeldItemSection(decodeSaveContainer(bytes));
    *destination = std::move(staged);
}

} // namespace mm6
