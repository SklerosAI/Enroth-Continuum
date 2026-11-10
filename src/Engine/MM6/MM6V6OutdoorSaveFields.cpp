#include "MM6V6OutdoorSaveFields.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "MM6ActiveActorSection.h"

namespace mm6 {
namespace {

constexpr std::array<char, 4> VIST = {'V', 'I', 'S', 'T'};
constexpr std::uint8_t VERSION = 1;
constexpr std::size_t MAX_VISITED = 15;
constexpr std::size_t OWNER_PARTS = 8;
constexpr std::size_t MAX_PAYLOAD = 32 * 1024 * 1024;

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 v6 outdoor owner requires MM6 title");
}

void put32(std::vector<std::uint8_t> *out, std::size_t value) {
    if (value > MAX_PAYLOAD)
        throw std::runtime_error("MM6 visited owner part exceeds bound");
    for (int shift = 0; shift < 32; shift += 8)
        out->push_back(static_cast<std::uint8_t>(value >> shift));
}

class Reader {
 public:
    explicit Reader(std::span<const std::uint8_t> bytes) : _bytes(bytes) {}

    std::uint8_t u8() {
        if (_offset == _bytes.size())
            throw std::runtime_error("truncated MM6 visited outdoor owner");
        return _bytes[_offset++];
    }

    std::uint32_t u32() {
        std::uint32_t value = 0;
        for (int shift = 0; shift < 32; shift += 8)
            value |= std::uint32_t(u8()) << shift;
        return value;
    }

    std::span<const std::uint8_t> take(std::size_t count) {
        if (count > _bytes.size() - _offset)
            throw std::runtime_error("truncated MM6 visited outdoor part");
        const auto part = _bytes.subspan(_offset, count);
        _offset += count;
        return part;
    }

    void finish() const {
        if (_offset != _bytes.size())
            throw std::runtime_error("MM6 visited outdoor trailing bytes");
    }

 private:
    std::span<const std::uint8_t> _bytes;
    std::size_t _offset = 0;
};

std::array<SaveSection, OWNER_PARTS> partsOf(
    const VisitedOutdoorRuntimeOwner &owner, GameTitle title) {
    requireVisitedOutdoorRuntimeWireOwner(owner, owner.mapName, title);
    const auto baseline = decodeActiveActorSection(owner.actorPayload, title);
    if (encodeActiveActorSection(baseline, title) != owner.actorPayload)
        throw std::runtime_error("MM6 visited ACTR is not canonical");
    return {encodeMapSection({{owner.nativeDelta}}),
            SaveSection{{'A', 'C', 'T', 'R'}, true, owner.actorPayload},
            encodeGeneratedNpcSection(owner.generatedNpcs, title),
            encodeChestSection(owner.chest),
            encodeEventRuntimeSection({{owner.event}}, title),
            encodeActorRuntimeSection(owner.actors, title),
            encodeSpriteRuntimeSection(owner.sprites, title),
            encodeDecorationRuntimeSection(owner.decorations, title)};
}

void requireVisitedOrder(std::span<const VisitedOutdoorRuntimeOwner> visited,
                         GameTitle title) {
    requireMM6(title);
    if (visited.empty() || visited.size() > MAX_VISITED)
        throw std::runtime_error("MM6 visited outdoor map count is invalid");
    std::string_view previous;
    for (const auto &owner : visited) {
        if (!previous.empty() && owner.mapName <= previous)
            throw std::runtime_error("MM6 visited outdoor map order differs");
        requireVisitedOutdoorRuntimeWireOwner(owner, owner.mapName, title);
        previous = owner.mapName;
    }
}

void requireActiveIsSeparate(const V6OutdoorSaveFields &fields) {
    const std::string &active = fields.active.v4.v3.raw.metadata.header.mapFileName;
    for (const auto &owner : fields.visited)
        if (owner.mapName == active)
            throw std::runtime_error("MM6 visited outdoor duplicates active map");
}

} // namespace

SaveSection encodeVisitedOutdoorSection(
    std::span<const VisitedOutdoorRuntimeOwner> visited, GameTitle title) {
    requireVisitedOrder(visited, title);
    SaveSection section{VIST, true, {VERSION,
        static_cast<std::uint8_t>(visited.size())}};
    for (const auto &owner : visited) {
        section.payload.push_back(static_cast<std::uint8_t>(owner.mapName.size()));
        section.payload.insert(section.payload.end(), owner.mapName.begin(),
                               owner.mapName.end());
        for (const auto &part : partsOf(owner, title)) {
            put32(&section.payload, part.payload.size());
            section.payload.insert(section.payload.end(), part.payload.begin(),
                                   part.payload.end());
            if (section.payload.size() > MAX_PAYLOAD)
                throw std::runtime_error("MM6 visited outdoor section exceeds bound");
        }
    }
    return section;
}

std::vector<VisitedOutdoorRuntimeOwner> decodeVisitedOutdoorSection(
    const SaveContainer &container, GameTitle title) {
    requireMM6(title);
    const auto it = std::find_if(container.sections.begin(), container.sections.end(),
        [](const SaveSection &section) { return section.tag == VIST; });
    if (it == container.sections.end() || !it->required ||
        it->payload.size() > MAX_PAYLOAD)
        throw std::runtime_error("MM6 v6 outdoor lacks bounded VIST owner");
    Reader reader(it->payload);
    if (reader.u8() != VERSION)
        throw std::runtime_error("unsupported MM6 visited outdoor version");
    const std::size_t count = reader.u8();
    if (count == 0 || count > MAX_VISITED)
        throw std::runtime_error("MM6 visited outdoor count is invalid");
    std::vector<VisitedOutdoorRuntimeOwner> visited;
    visited.reserve(count);
    constexpr std::array<std::array<char, 4>, OWNER_PARTS> TAGS = {{
        {{'M', 'A', 'P', 'S'}}, {{'A', 'C', 'T', 'R'}}, {{'G', 'N', 'P', 'C'}},
        {{'C', 'H', 'S', 'T'}}, {{'E', 'V', 'R', 'T'}}, {{'A', 'R', 'T', 'R'}},
        {{'S', 'P', 'R', 'T'}}, {{'D', 'E', 'C', 'R'}}}};
    for (std::size_t i = 0; i < count; ++i) {
        const auto nameBytes = reader.take(reader.u8());
        const std::string name(nameBytes.begin(), nameBytes.end());
        SaveContainer inner;
        inner.version = 6;
        for (const auto &tag : TAGS) {
            const std::uint32_t size = reader.u32();
            if (size > MAX_PAYLOAD)
                throw std::runtime_error("MM6 visited outdoor part exceeds bound");
            const auto bytes = reader.take(size);
            inner.sections.push_back({tag, true, {bytes.begin(), bytes.end()}});
        }
        const MapSaveFields maps = decodeMapSection(inner);
        const EventRuntimeSaveFields events = decodeEventRuntimeSection(inner, title);
        if (maps.deltas.size() != 1 || events.maps.size() != 1)
            throw std::runtime_error("MM6 visited owner has multiple inner maps");
        VisitedOutdoorRuntimeOwner owner{
            name, maps.deltas.front(), inner.sections[1].payload,
            decodeGeneratedNpcSection(inner, title), decodeChestSection(inner),
            events.maps.front(), decodeActorRuntimeSection(inner, title),
            decodeSpriteRuntimeSection(inner, title),
            decodeDecorationRuntimeSection(inner, title)};
        requireVisitedOutdoorRuntimeWireOwner(owner, name, title);
        if (partsOf(owner, title) != std::array<SaveSection, OWNER_PARTS>{
                inner.sections[0], inner.sections[1], inner.sections[2],
                inner.sections[3], inner.sections[4], inner.sections[5],
                inner.sections[6], inner.sections[7]})
            throw std::runtime_error("MM6 visited outdoor owner is not canonical");
        visited.push_back(std::move(owner));
    }
    reader.finish();
    requireVisitedOrder(visited, title);
    if (encodeVisitedOutdoorSection(visited, title) != *it)
        throw std::runtime_error("MM6 visited outdoor section is not canonical");
    return visited;
}

std::vector<std::uint8_t> encodeV6OutdoorSave(
    const V6OutdoorSaveFields &fields, GameTitle title) {
    requireMM6(title);
    requireVisitedOrder(fields.visited, title);
    requireActiveIsSeparate(fields);
    SaveContainer container = decodeSaveContainer(encodeV5Save(fields.active, title));
    container.version = 6;
    container.sections.push_back(encodeVisitedOutdoorSection(fields.visited, title));
    if (container.sections.size() != 27)
        throw std::runtime_error("MM6 v6 outdoor requires 27 exact sections");
    return encodeSaveContainer(container);
}

V6OutdoorSaveFields decodeV6OutdoorSave(
    std::span<const std::uint8_t> bytes, GameTitle title) {
    requireMM6(title);
    SaveContainer container = decodeSaveContainer(bytes);
    if (container.version != 6 || container.sections.size() != 27 ||
        container.sections.back().tag != VIST || !container.sections.back().required)
        throw std::runtime_error("MM6 v6 outdoor requires active v5 plus VIST");
    auto visited = decodeVisitedOutdoorSection(container, title);
    container.sections.pop_back();
    container.version = 5;
    V6OutdoorSaveFields result{decodeV5Save(encodeSaveContainer(container), title),
                               std::move(visited)};
    requireActiveIsSeparate(result);
    if (encodeV6OutdoorSave(result, title) !=
        std::vector<std::uint8_t>(bytes.begin(), bytes.end()))
        throw std::runtime_error("MM6 v6 outdoor save is not canonical");
    return result;
}

} // namespace mm6
