#include "MM6VisitedMapSection.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "MM6V5SaveFields.h"
#include "MM6V6IndoorSaveFields.h"
#include "MM6V6OutdoorSaveFields.h"

namespace mm6 {

static constexpr std::array<char, 4> VIST = {'V', 'I', 'S', 'T'};
static constexpr std::array<char, 4> IACR = {'I', 'A', 'C', 'R'};
static constexpr std::uint8_t VERSION = 2;
static constexpr std::uint8_t KIND_OUTDOOR = 0;
static constexpr std::uint8_t KIND_INDOOR = 1;
static constexpr std::size_t MAX_PAYLOAD = 32 * 1024 * 1024; // Section size bound of the EC6 container.

static constexpr std::array<char, 4> MAPS = {'M', 'A', 'P', 'S'};
static constexpr std::array<char, 4> GNPC = {'G', 'N', 'P', 'C'};
static constexpr std::array<char, 4> CHST = {'C', 'H', 'S', 'T'};
static constexpr std::array<char, 4> EVRT = {'E', 'V', 'R', 'T'};
static constexpr std::array<char, 4> IFOV = {'I', 'F', 'O', 'V'};
static constexpr std::array<char, 4> ISCR = {'I', 'S', 'C', 'R'};
static constexpr std::array<char, 4> IDCR = {'I', 'D', 'C', 'R'};
static constexpr std::array<char, 4> IDRT = {'I', 'D', 'R', 'T'};
static constexpr std::array<char, 4> ISRT = {'I', 'S', 'R', 'T'};
static constexpr std::array<char, 4> IART = {'I', 'A', 'R', 'T'};
static constexpr std::array<std::array<char, 4>, 11> INDOOR_TAGS = {MAPS, GNPC, CHST, EVRT, IFOV, IACR, ISCR, IDCR, IDRT, ISRT, IART};
static constexpr std::size_t IFOV_PART = 4;

static void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 visited map section requires MM6 title");
}

static void put32(std::vector<std::uint8_t> *out, std::size_t value) {
    if (value > MAX_PAYLOAD)
        throw std::runtime_error("MM6 visited map part exceeds bound");
    for (int shift = 0; shift < 32; shift += 8)
        out->push_back(static_cast<std::uint8_t>(value >> shift));
}

static void putBytes(std::vector<std::uint8_t> *out, std::span<const std::uint8_t> bytes) {
    put32(out, bytes.size());
    out->insert(out->end(), bytes.begin(), bytes.end());
    if (out->size() > MAX_PAYLOAD)
        throw std::runtime_error("MM6 visited map section exceeds bound");
}

class VisitedMapReader {
 public:
    explicit VisitedMapReader(std::span<const std::uint8_t> bytes) : _bytes(bytes) {}

    std::uint8_t u8() {
        if (_offset == _bytes.size())
            throw std::runtime_error("truncated MM6 visited map section");
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
            throw std::runtime_error("truncated MM6 visited map part");
        const auto part = _bytes.subspan(_offset, count);
        _offset += count;
        return part;
    }

    std::span<const std::uint8_t> sized() {
        return take(u32());
    }

    void finish() const {
        if (_offset != _bytes.size())
            throw std::runtime_error("MM6 visited map section has trailing bytes");
    }

 private:
    std::span<const std::uint8_t> _bytes;
    std::size_t _offset = 0;
};

static void putSection(std::vector<std::uint8_t> *out, const SaveSection &section) {
    out->insert(out->end(), section.tag.begin(), section.tag.end());
    out->push_back(section.required ? 1 : 0);
    putBytes(out, section.payload);
}

static SaveSection readSection(VisitedMapReader *reader, const std::array<char, 4> &tag) {
    SaveSection section{{}, true, {}};
    for (char &c : section.tag)
        c = static_cast<char>(reader->u8());
    const std::uint8_t required = reader->u8();
    if (section.tag != tag || required > 1)
        throw std::runtime_error("MM6 visited indoor part is out of order");
    section.required = required;
    const auto payload = reader->sized();
    section.payload.assign(payload.begin(), payload.end());
    return section;
}

// Same layout as the v6 indoor IACR, ISCR and IDCR sections.
static SaveSection encodeAnchor(const IndoorNativeSlotAnchor &anchor, const std::array<char, 4> &tag) {
    SaveSection section{tag, true, {1, static_cast<std::uint8_t>(anchor.mapName.size())}};
    section.payload.insert(section.payload.end(), anchor.mapName.begin(), anchor.mapName.end());
    for (int shift = 0; shift < 32; shift += 8)
        section.payload.push_back(static_cast<std::uint8_t>(anchor.count >> shift));
    section.payload.insert(section.payload.end(), anchor.records.begin(), anchor.records.end());
    return section;
}

static IndoorNativeSlotAnchor decodeAnchor(const SaveSection &section) {
    VisitedMapReader reader(section.payload);
    if (reader.u8() != 1)
        throw std::runtime_error("unsupported MM6 visited indoor anchor version");
    IndoorNativeSlotAnchor anchor;
    const auto name = reader.take(reader.u8());
    anchor.mapName.assign(name.begin(), name.end());
    anchor.count = reader.u32();
    const auto records = reader.take(section.payload.size() - 2 - name.size() - 4);
    anchor.records.assign(records.begin(), records.end());
    return anchor;
}

static std::vector<SaveSection> indoorParts(const VisitedIndoorRuntimeOwner &owner, GameTitle title) {
    requireVisitedIndoorRuntimeWireOwner(owner, owner.mapName, title);
    std::vector<SaveSection> parts{
        encodeMapSection({{owner.nativeDelta}}),
        encodeGeneratedNpcSection(owner.generatedNpcs, title),
        encodeChestSection(owner.chest),
        encodeEventRuntimeSection({{owner.event}}, title)};
    if (owner.faceOverrides)
        parts.push_back(encodeIndoorFaceOverrideSection({{*owner.faceOverrides}}));
    parts.push_back(encodeAnchor(owner.nativeActors, IACR));
    parts.push_back(encodeAnchor(owner.nativeSprites, ISCR));
    parts.push_back(encodeAnchor(owner.nativeDecorations, IDCR));
    parts.push_back(encodeIndoorDecorationRuntimeSection(owner.decorations, title));
    parts.push_back(encodeSpriteRuntimeSectionAs(owner.sprites, SpriteRuntimeSectionKind::IndoorV6, title));
    parts.push_back(encodeActorRuntimeSectionAs(owner.actors, ActorRuntimeSectionKind::IndoorV6, title));
    return parts;
}

static std::vector<std::uint8_t> encodeIndoorEntry(const VisitedIndoorRuntimeOwner &owner, GameTitle title) {
    const std::vector<SaveSection> parts = indoorParts(owner, title);
    std::vector<std::uint8_t> out{static_cast<std::uint8_t>(owner.mapName.size())};
    out.insert(out.end(), owner.mapName.begin(), owner.mapName.end());
    out.push_back(owner.faceOverrides ? 1 : 0);
    for (const SaveSection &part : parts)
        putSection(&out, part);
    return out;
}

static VisitedIndoorRuntimeOwner decodeIndoorEntry(std::span<const std::uint8_t> bytes, GameTitle title) {
    VisitedMapReader reader(bytes);
    const auto nameBytes = reader.take(reader.u8());
    const std::uint8_t hasFaces = reader.u8();
    if (hasFaces > 1)
        throw std::runtime_error("MM6 visited indoor face flag is invalid");
    SaveContainer inner;
    inner.version = 6;
    for (std::size_t part = 0; part < INDOOR_TAGS.size(); ++part)
        if (part != IFOV_PART || hasFaces)
            inner.sections.push_back(readSection(&reader, INDOOR_TAGS[part]));
    reader.finish();

    const auto partOf = [&](const std::array<char, 4> &tag) -> const SaveSection & {
        return *std::find_if(inner.sections.begin(), inner.sections.end(),
                             [&](const SaveSection &section) { return section.tag == tag; });
    };
    const MapSaveFields maps = decodeMapSection(inner);
    const EventRuntimeSaveFields events = decodeEventRuntimeSection(inner, title);
    const IndoorFaceOverrides faces = decodeIndoorFaceOverrideSection(inner);
    if (maps.deltas.size() != 1 || events.maps.size() != 1 || faces.maps.size() != hasFaces)
        throw std::runtime_error("MM6 visited indoor owner has multiple inner maps");
    VisitedIndoorRuntimeOwner owner{
        std::string(nameBytes.begin(), nameBytes.end()), maps.deltas.front(),
        decodeGeneratedNpcSection(inner, title), decodeChestSection(inner), events.maps.front(),
        hasFaces ? std::optional(faces.maps.front()) : std::nullopt,
        decodeAnchor(partOf(IACR)), decodeAnchor(partOf(ISCR)),
        decodeAnchor(partOf(IDCR)),
        decodeIndoorDecorationRuntimeSection(inner, title),
        decodeSpriteRuntimeSectionAs(inner, SpriteRuntimeSectionKind::IndoorV6, title),
        decodeActorRuntimeSectionAs(inner, ActorRuntimeSectionKind::IndoorV6, title)};
    if (encodeIndoorEntry(owner, title) != std::vector<std::uint8_t>(bytes.begin(), bytes.end()))
        throw std::runtime_error("MM6 visited indoor owner is not canonical");
    return owner;
}

static std::vector<std::uint8_t> encodeOutdoorEntry(const VisitedOutdoorRuntimeOwner &owner, GameTitle title) {
    return encodeVisitedOutdoorSection(std::span(&owner, 1), title).payload;
}

static VisitedOutdoorRuntimeOwner decodeOutdoorEntry(std::span<const std::uint8_t> bytes, GameTitle title) {
    const SaveContainer inner{{{VIST, true, {bytes.begin(), bytes.end()}}}, 6};
    std::vector<VisitedOutdoorRuntimeOwner> owners = decodeVisitedOutdoorSection(inner, title);
    if (owners.size() != 1)
        throw std::runtime_error("MM6 visited outdoor entry holds several maps");
    return std::move(owners.front());
}

SaveSection encodeVisitedMapSection(const VisitedMapCache &visited, std::string_view activeMap, GameTitle title) {
    requireMM6(title);
    visited.requireSeparateFrom(activeMap);
    if (visited.size() > VisitedMapCache::MAX_MAPS)
        throw std::runtime_error("MM6 visited map count is invalid");
    SaveSection section{VIST, true, {VERSION, static_cast<std::uint8_t>(visited.size())}};
    for (const VisitedMapOwner &owner : visited.entries()) {
        if (const auto *outdoor = std::get_if<VisitedOutdoorRuntimeOwner>(&owner)) {
            section.payload.push_back(KIND_OUTDOOR);
            putBytes(&section.payload, encodeOutdoorEntry(*outdoor, title));
        } else {
            section.payload.push_back(KIND_INDOOR);
            putBytes(&section.payload, encodeIndoorEntry(std::get<VisitedIndoorRuntimeOwner>(owner), title));
        }
    }
    return section;
}

VisitedMapCache decodeVisitedMapSection(const SaveContainer &container, std::string_view activeMap, GameTitle title) {
    requireMM6(title);
    const auto it = std::find_if(container.sections.begin(), container.sections.end(),
        [](const SaveSection &section) { return section.tag == VIST; });
    if (it == container.sections.end() || !it->required || it->payload.size() > MAX_PAYLOAD)
        throw std::runtime_error("MM6 session lacks bounded VIST owner");
    VisitedMapReader reader(it->payload);
    if (reader.u8() != VERSION)
        throw std::runtime_error("unsupported MM6 visited map version");
    const std::size_t count = reader.u8();
    if (count > VisitedMapCache::MAX_MAPS)
        throw std::runtime_error("MM6 visited map count is invalid");
    VisitedMapCache visited;
    std::string previous;
    for (std::size_t i = 0; i < count; ++i) {
        const std::uint8_t kind = reader.u8();
        const auto bytes = reader.sized();
        VisitedMapOwner owner;
        if (kind == KIND_OUTDOOR) {
            owner = decodeOutdoorEntry(bytes, title);
        } else if (kind == KIND_INDOOR) {
            owner = decodeIndoorEntry(bytes, title);
        } else {
            throw std::runtime_error("MM6 visited map kind is invalid");
        }
        const std::string_view name = visitedMapName(owner);
        if (i > 0 && name <= previous)
            throw std::runtime_error("MM6 visited map order differs");
        previous = name;
        visited.park(std::move(owner), title);
    }
    reader.finish();
    if (encodeVisitedMapSection(visited, activeMap, title) != *it)
        throw std::runtime_error("MM6 visited map section is not canonical");
    return visited;
}

static std::pair<ActiveMapBranch, std::string> activeMapOf(std::span<const std::uint8_t> active, GameTitle title) {
    const SaveContainer container = decodeSaveContainer(active);
    const bool indoor = std::any_of(container.sections.begin(), container.sections.end(),
        [](const SaveSection &section) { return section.tag == IACR; });
    if (container.version == 5 && !indoor)
        return {ActiveMapBranch::OutdoorV5, decodeV5Save(active, title).v4.v3.raw.metadata.header.mapFileName};
    if (container.version == 6 && indoor)
        return {ActiveMapBranch::IndoorV6, decodeV6IndoorSave(active, title).raw.metadata.header.mapFileName};
    throw std::runtime_error("MM6 session lacks an outdoor v5 or indoor v6 active map");
}

std::vector<std::uint8_t> encodeSessionWithVisitedMaps(std::span<const std::uint8_t> active,
                                                       const VisitedMapCache &visited, GameTitle title) {
    requireMM6(title);
    const std::string activeMap = activeMapOf(active, title).second;
    SaveContainer container = decodeSaveContainer(active);
    container.version = 6;
    container.sections.push_back(encodeVisitedMapSection(visited, activeMap, title));
    return encodeSaveContainer(container);
}

SessionWithVisitedMaps decodeSessionWithVisitedMaps(std::span<const std::uint8_t> bytes, GameTitle title) {
    requireMM6(title);
    SaveContainer container = decodeSaveContainer(bytes);
    if (container.version != 6 || container.sections.empty() || container.sections.back().tag != VIST ||
        container.sections.back().payload.empty() || container.sections.back().payload.front() != VERSION)
        throw std::runtime_error("MM6 session requires an active map plus VIST version 2");
    const SaveContainer visitedPart{{std::move(container.sections.back())}, 6};
    container.sections.pop_back();
    const bool indoor = std::any_of(container.sections.begin(), container.sections.end(),
        [](const SaveSection &section) { return section.tag == IACR; });
    container.version = indoor ? 6 : 5;

    SessionWithVisitedMaps result;
    result.active = encodeSaveContainer(container);
    std::tie(result.branch, result.activeMap) = activeMapOf(result.active, title);
    result.visited = decodeVisitedMapSection(visitedPart, result.activeMap, title);
    if (encodeSessionWithVisitedMaps(result.active, result.visited, title) !=
        std::vector<std::uint8_t>(bytes.begin(), bytes.end()))
        throw std::runtime_error("MM6 session with visited maps is not canonical");
    return result;
}

} // namespace mm6
