#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace mm6 {

// Original MM6 item storage. Keep this value as the authority across shop,
// inventory, chest, ground and cursor contexts until each live adapter proves
// its own semantics. In particular, bytes 26 and 27 are not MM7 item fields.
struct ItemRecord28 {
    std::array<std::uint8_t, 28> bytes = {};

    constexpr auto begin() { return bytes.begin(); }
    constexpr auto begin() const { return bytes.begin(); }
    constexpr auto end() { return bytes.end(); }
    constexpr auto end() const { return bytes.end(); }
    constexpr std::size_t size() const { return bytes.size(); }
    constexpr std::uint8_t &operator[](std::size_t index) { return bytes[index]; }
    constexpr const std::uint8_t &operator[](std::size_t index) const { return bytes[index]; }
    constexpr std::uint8_t &back() { return bytes.back(); }
    constexpr const std::uint8_t &back() const { return bytes.back(); }

    // These are structural little-endian field views. No category-dependent
    // bonus or flag meaning is inferred from the raw words.
    constexpr std::uint32_t word(std::size_t offset) const {
        if (offset > 24 || offset % 4 != 0)
            throw std::out_of_range("MM6 item word offset");
        return std::uint32_t(bytes[offset]) |
               (std::uint32_t(bytes[offset + 1]) << 8) |
               (std::uint32_t(bytes[offset + 2]) << 16) |
               (std::uint32_t(bytes[offset + 3]) << 24);
    }

    constexpr void setWord(std::size_t offset, std::uint32_t value) {
        if (offset > 24 || offset % 4 != 0)
            throw std::out_of_range("MM6 item word offset");
        for (std::size_t i = 0; i < 4; ++i)
            bytes[offset + i] = static_cast<std::uint8_t>(value >> (8 * i));
    }

    constexpr std::uint32_t itemNumberRaw() const { return word(0); }
    constexpr std::uint32_t bonusRaw() const { return word(4); }
    constexpr std::uint32_t bonusStrengthRaw() const { return word(8); }
    constexpr std::uint32_t secondBonusRaw() const { return word(12); }
    constexpr std::uint32_t chargesRaw() const { return word(16); }
    constexpr std::uint32_t conditionRaw() const { return word(20); }

    constexpr std::uint8_t bodyLocationRaw() const { return bytes[24]; }
    constexpr std::uint8_t maxChargesRaw() const { return bytes[25]; }
    constexpr std::uint8_t ownerByteRaw() const { return bytes[26]; }
    constexpr std::uint8_t paddingByteRaw() const { return bytes[27]; }

    bool operator==(const ItemRecord28 &) const = default;
};

static_assert(sizeof(ItemRecord28) == 28);

} // namespace mm6
