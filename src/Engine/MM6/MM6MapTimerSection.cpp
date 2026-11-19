#include "MM6MapTimerSection.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace mm6 {

static constexpr std::array<char, 4> TIMR = {'T', 'I', 'M', 'R'};
static constexpr std::uint8_t VERSION = 1;
static constexpr std::size_t MAX_MAPS = 67;

static void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 TIMR requires the MM6 title");
}

static bool isCanonicalMapName(std::string_view name) {
    return name.size() > 4 && name.size() <= 63 && (name.ends_with(".odm") || name.ends_with(".blv")) &&
           std::ranges::all_of(name, [](unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.'; });
}

static void writeU64(std::vector<std::uint8_t> *bytes, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8)
        bytes->push_back(static_cast<std::uint8_t>(value >> shift));
}

static std::uint64_t readU64(std::span<const std::uint8_t> bytes, std::size_t *offset) {
    if (bytes.size() - *offset < 8)
        throw std::runtime_error("truncated MM6 TIMR section");
    std::uint64_t result = 0;
    for (int shift = 0; shift < 64; shift += 8)
        result |= std::uint64_t(bytes[(*offset)++]) << shift;
    return result;
}

SaveSection encodeMapTimerSection(const MapTimerFirings &firings, GameTitle title) {
    requireMM6(title);
    if (firings.size() > MAX_MAPS)
        throw std::runtime_error("MM6 TIMR holds more maps than the title has");
    SaveSection section{TIMR, false, {VERSION, static_cast<std::uint8_t>(firings.size())}};
    for (const auto &[mapName, lastFirings] : firings) {
        if (!isCanonicalMapName(mapName))
            throw std::runtime_error("MM6 TIMR map name is not canonical");
        section.payload.push_back(static_cast<std::uint8_t>(mapName.size()));
        section.payload.insert(section.payload.end(), mapName.begin(), mapName.end());
        for (Time firing : lastFirings) {
            if (firing.ticks() < 0)
                throw std::runtime_error("MM6 TIMR firing is before the game began");
            writeU64(&section.payload, std::bit_cast<std::uint64_t>(firing.ticks()));
        }
    }
    return section;
}

MapTimerFirings decodeMapTimerSection(const SaveContainer &container, GameTitle title) {
    requireMM6(title);
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections) {
        if (candidate.tag != TIMR)
            continue;
        if (section)
            throw std::runtime_error("duplicate MM6 TIMR section");
        section = &candidate;
    }
    MapTimerFirings result;
    if (!section)
        return result;

    const std::span<const std::uint8_t> bytes = section->payload;
    if (section->required || bytes.size() < 2 || bytes[0] != VERSION || bytes[1] > MAX_MAPS)
        throw std::runtime_error("malformed MM6 TIMR header");
    std::size_t offset = 2;
    for (int map = 0; map < bytes[1]; ++map) {
        if (offset >= bytes.size() || bytes[offset] > bytes.size() - offset - 1)
            throw std::runtime_error("truncated MM6 TIMR section");
        const std::string mapName(reinterpret_cast<const char *>(bytes.data() + offset + 1), bytes[offset]);
        offset += 1 + mapName.size();
        if (!isCanonicalMapName(mapName) || (!result.empty() && result.rbegin()->first >= mapName))
            throw std::runtime_error("MM6 TIMR map names are not canonical");
        std::array<Time, 4> &lastFirings = result[mapName];
        for (Time &firing : lastFirings) {
            firing = Time::fromTicks(std::bit_cast<std::int64_t>(readU64(bytes, &offset)));
            if (firing.ticks() < 0)
                throw std::runtime_error("MM6 TIMR firing is before the game began");
        }
    }
    if (offset != bytes.size())
        throw std::runtime_error("MM6 TIMR has trailing bytes");
    return result;
}

} // namespace mm6
