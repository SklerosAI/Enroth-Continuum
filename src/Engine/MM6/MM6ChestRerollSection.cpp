#include "MM6ChestRerollSection.h"

#include <algorithm>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mm6 {

static constexpr std::array<char, 4> CHRL = {'C', 'H', 'R', 'L'};
static constexpr std::uint8_t VERSION = 1;
static constexpr std::size_t MAX_CHESTS = 67 * 20;

static void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 CHRL requires the MM6 title");
}

static bool isCanonicalMapName(std::string_view name) {
    return name.size() > 4 && name.size() <= 63 && (name.ends_with(".odm") || name.ends_with(".blv")) &&
           std::ranges::all_of(name, [](unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.'; });
}

static void requireEntry(const std::pair<std::string, int> &key, const ChestRerollLayout &layout) {
    if (!isCanonicalMapName(key.first) || key.second < 0 || key.second >= 20)
        throw std::runtime_error("MM6 CHRL chest is not on a canonical map");
    const bool valid = std::ranges::all_of(layout, [](std::int8_t slot) { return slot >= -7 && slot <= 1; });
    const bool marker = std::ranges::any_of(layout, [](std::int8_t slot) { return slot < 0; });
    if (!valid || !marker)
        throw std::runtime_error("MM6 CHRL layout holds no random-loot marker");
}

static ChestRerollLayouts decodePayload(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 3 || bytes[0] != VERSION)
        throw std::runtime_error("malformed MM6 CHRL header");
    const std::size_t count = bytes[1] | (bytes[2] << 8);
    if (count == 0 || count > MAX_CHESTS)
        throw std::runtime_error("MM6 CHRL holds no chest or more chests than the title has");
    ChestRerollLayouts result;
    std::size_t offset = 3;
    for (std::size_t chest = 0; chest < count; ++chest) {
        if (bytes.size() - offset < 1 || bytes.size() - offset - 1 < bytes[offset] + 1u + 140u)
            throw std::runtime_error("truncated MM6 CHRL section");
        const std::size_t nameSize = bytes[offset++];
        std::pair<std::string, int> key(std::string(bytes.begin() + offset, bytes.begin() + offset + nameSize), 0);
        offset += nameSize;
        key.second = bytes[offset++];
        ChestRerollLayout layout;
        for (std::int8_t &slot : layout)
            slot = static_cast<std::int8_t>(bytes[offset++]);
        requireEntry(key, layout);
        if (!result.empty() && !(result.rbegin()->first < key))
            throw std::runtime_error("MM6 CHRL chests are out of order");
        result.emplace_hint(result.end(), std::move(key), layout);
    }
    if (offset != bytes.size())
        throw std::runtime_error("MM6 CHRL section has trailing bytes");
    return result;
}

std::optional<SaveSection> encodeChestRerollSection(const ChestRerollLayouts &layouts, GameTitle title) {
    requireMM6(title);
    if (layouts.empty())
        return std::nullopt;
    if (layouts.size() > MAX_CHESTS)
        throw std::runtime_error("MM6 CHRL holds more chests than the title has");
    SaveSection section{CHRL, false, {VERSION, static_cast<std::uint8_t>(layouts.size()), static_cast<std::uint8_t>(layouts.size() >> 8)}};
    for (const auto &[key, layout] : layouts) {
        requireEntry(key, layout);
        section.payload.push_back(static_cast<std::uint8_t>(key.first.size()));
        section.payload.insert(section.payload.end(), key.first.begin(), key.first.end());
        section.payload.push_back(static_cast<std::uint8_t>(key.second));
        for (std::int8_t slot : layout)
            section.payload.push_back(static_cast<std::uint8_t>(slot));
    }
    if (decodePayload(section.payload) != layouts)
        throw std::runtime_error("MM6 CHRL doesn't round-trip");
    return section;
}

ChestRerollLayouts decodeChestRerollSection(const SaveContainer &container, GameTitle title) {
    requireMM6(title);
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections) {
        if (candidate.tag != CHRL)
            continue;
        if (section)
            throw std::runtime_error("duplicate MM6 CHRL section");
        section = &candidate;
    }
    if (!section)
        return {};
    if (section->required)
        throw std::runtime_error("MM6 CHRL section is marked required");
    return decodePayload(section->payload);
}

} // namespace mm6
