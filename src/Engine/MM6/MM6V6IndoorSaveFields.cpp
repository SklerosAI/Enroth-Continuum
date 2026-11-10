#include "MM6V6IndoorSaveFields.h"

#include <algorithm>
#include <array>
#include <deque>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Engine/Snapshots/MM6IndoorSnapshot.h"

#include "MM6RawSaveFields.h"

namespace mm6 {
namespace {

constexpr std::array<char, 4> IACR = {'I', 'A', 'C', 'R'};
constexpr std::array<char, 4> ISCR = {'I', 'S', 'C', 'R'};
constexpr std::array<char, 4> IDCR = {'I', 'D', 'C', 'R'};
constexpr std::array<char, 4> IDRT = {'I', 'D', 'R', 'T'};
constexpr std::array<char, 4> ISRT = {'I', 'S', 'R', 'T'};
constexpr std::array<char, 4> IART = {'I', 'A', 'R', 'T'};
constexpr std::array<char, 4> IFOV = {'I', 'F', 'O', 'V'};
constexpr std::array<char, 4> GNPC = {'G', 'N', 'P', 'C'};
constexpr std::array<char, 4> RNGS = {'R', 'N', 'G', 'S'};
constexpr std::array<char, 4> EVRT = {'E', 'V', 'R', 'T'};
constexpr std::array<std::array<char, 4>, 10> NEW_TAGS = {GNPC, RNGS, EVRT, IFOV, IACR, ISCR, IDCR, IDRT, ISRT, IART};
constexpr std::size_t MAX_MAP_NAME = 63;
constexpr std::size_t MAX_ACTORS = 8192;
constexpr std::size_t MAX_SPRITES = 4096;
constexpr std::size_t MAX_DECORATIONS = 16384;

bool isNewTag(const std::array<char, 4> &tag) {
    return std::find(NEW_TAGS.begin(), NEW_TAGS.end(), tag) != NEW_TAGS.end();
}

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 v6 indoor save requires MM6 title");
}

void requireMapName(std::string_view name) {
    if (name.size() < 5 || name.size() > MAX_MAP_NAME || !name.ends_with(".blv") ||
        !std::all_of(name.begin(), name.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                   c == '_' || c == '.';
        }))
        throw std::runtime_error("MM6 v6 native owner lacks canonical BLV identity");
}

void put32(std::vector<std::uint8_t> *out, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        out->push_back(value >> shift);
}

std::uint32_t read32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || 4 > bytes.size() - offset)
        throw std::runtime_error("truncated MM6 v6 native owner");
    return std::uint32_t(bytes[offset]) | std::uint32_t(bytes[offset + 1]) << 8 |
           std::uint32_t(bytes[offset + 2]) << 16 |
           std::uint32_t(bytes[offset + 3]) << 24;
}

void requireManifest(const SaveContainer &container) {
    if (container.version != 6 ||
        (container.sections.size() != 28 && container.sections.size() != 29))
        throw std::runtime_error("MM6 v6 indoor save has an invalid section count");
    for (const auto &tag : NEW_TAGS) {
        const auto it = std::find_if(container.sections.begin(), container.sections.end(),
            [&tag](const SaveSection &section) { return section.tag == tag; });
        if ((tag != IFOV && it == container.sections.end()) ||
            (it != container.sections.end() && it->required != (tag != IFOV)))
            throw std::runtime_error("MM6 v6 indoor save lacks a required owner");
    }
    const bool hasIfov = std::any_of(container.sections.begin(), container.sections.end(),
        [](const SaveSection &section) { return section.tag == IFOV; });
    if (container.sections.size() != 28 + hasIfov)
        throw std::runtime_error("MM6 v6 indoor save has an unexpected section");
}

SaveSection encodeAnchor(const IndoorNativeSlotAnchor &owner,
                         const std::array<char, 4> &tag, std::size_t width,
                         std::size_t maxCount) {
    requireMapName(owner.mapName);
    if (owner.count > maxCount || owner.records.size() != owner.count * width)
        throw std::runtime_error("MM6 v6 indoor native owner has invalid record shape");
    SaveSection section{tag, true, {}};
    section.payload.reserve(2 + owner.mapName.size() + 4 + owner.records.size());
    section.payload.push_back(1);
    section.payload.push_back(owner.mapName.size());
    section.payload.insert(section.payload.end(), owner.mapName.begin(), owner.mapName.end());
    put32(&section.payload, owner.count);
    section.payload.insert(section.payload.end(), owner.records.begin(), owner.records.end());
    return section;
}

IndoorNativeSlotAnchor decodeAnchor(const SaveContainer &container,
                                    const std::array<char, 4> &tag,
                                    std::size_t width, std::size_t maxCount) {
    const auto it = std::find_if(container.sections.begin(), container.sections.end(),
        [&tag](const SaveSection &section) { return section.tag == tag; });
    if (it == container.sections.end() || !it->required || it->payload.size() < 6 ||
        it->payload.size() > 2 + MAX_MAP_NAME + 4 + maxCount * width)
        throw std::runtime_error("missing or unbounded MM6 v6 indoor native owner");
    const auto &bytes = it->payload;
    const std::size_t nameSize = bytes[1];
    if (bytes[0] != 1 || nameSize > MAX_MAP_NAME ||
        2 + nameSize + 4 > bytes.size())
        throw std::runtime_error("MM6 v6 indoor native owner has invalid header");
    IndoorNativeSlotAnchor owner;
    owner.mapName.assign(bytes.begin() + 2, bytes.begin() + 2 + nameSize);
    requireMapName(owner.mapName);
    owner.count = read32(bytes, 2 + nameSize);
    if (owner.count > maxCount ||
        bytes.size() != 2 + nameSize + 4 + owner.count * width)
        throw std::runtime_error("MM6 v6 indoor native owner count differs from payload");
    owner.records.assign(bytes.begin() + 2 + nameSize + 4, bytes.end());
    if (encodeAnchor(owner, tag, width, maxCount) != *it)
        throw std::runtime_error("MM6 v6 indoor native owner is not canonical");
    return owner;
}

IndoorNativeSlotAnchor anchorFromWire(std::string_view mapName,
                                      const MM6IndoorSection &section,
                                      std::size_t width, std::size_t maxCount) {
    if (section.count > maxCount || section.bytes.size() != 4 + section.count * width)
        throw std::runtime_error("MM6 v6 indoor source has invalid native slot shape");
    IndoorNativeSlotAnchor owner;
    owner.mapName = mapName;
    owner.count = section.count;
    const auto *start = static_cast<const std::uint8_t *>(section.bytes.data()) + 4;
    owner.records.assign(start, start + section.count * width);
    return owner;
}

void requireNativeJoin(const V6IndoorSaveFields &fields,
                       const StagedRawSessionFields &represented) {
    if (!represented.indoorPlan || represented.outdoorPlan ||
        fields.raw.metadata.header.mapFileName != represented.indoorPlan->mapName ||
        fields.generatedNpcs.mapName != represented.indoorPlan->mapName)
        throw std::runtime_error("MM6 v6 indoor HEAD and represented world disagree");
    IndoorNativeSlotAnchor actors, sprites, decorations;
    captureV6IndoorNativeAnchors(represented, &actors, &sprites, &decorations);
    if (fields.actors != actors || fields.sprites != sprites ||
        fields.decorations != decorations)
        throw std::runtime_error("MM6 v6 indoor native slot owner differs from BLV/DLV");
    const auto active = std::find_if(fields.eventRuntime.maps.begin(),
                                    fields.eventRuntime.maps.end(),
        [&represented](const EventRuntimeMapEntry &entry) {
            return entry.mapName == represented.indoorPlan->mapName;
        });
    if (active == fields.eventRuntime.maps.end() ||
        active->respawnCount != represented.indoorPlan->activeSidecar.respawnCount ||
        active->lastRespawnDay != represented.indoorPlan->activeSidecar.lastRespawnDay)
        throw std::runtime_error("MM6 v6 indoor EVRT lacks active DLV reset owner");
}

} // namespace

void captureV6IndoorNativeAnchors(const StagedRawSessionFields &represented,
                                  IndoorNativeSlotAnchor *actors,
                                  IndoorNativeSlotAnchor *sprites,
                                  IndoorNativeSlotAnchor *decorations) {
    if (!actors || !sprites || !decorations || !represented.indoorPlan ||
        represented.outdoorPlan)
        throw std::runtime_error("MM6 v6 native capture requires detached indoor plan");
    const auto &plan = *represented.indoorPlan;
    const auto *active = std::get_if<StagedIndoorMap>(&represented.raw.activeMap);
    if (!active || active->mapName != plan.mapName)
        throw std::runtime_error("MM6 v6 native capture lacks active indoor source");
    *actors = anchorFromWire(plan.mapName,
        plan.savedDelta.wire.section(MM6IndoorSectionKind::Actors), 548, MAX_ACTORS);
    *sprites = anchorFromWire(plan.mapName,
        plan.savedDelta.wire.section(MM6IndoorSectionKind::GroundItems), 100, MAX_SPRITES);
    *decorations = anchorFromWire(plan.mapName,
        active->base.section(MM6IndoorSectionKind::Sprites), 60, MAX_DECORATIONS);
}

std::vector<std::uint8_t> encodeV6IndoorSave(const V6IndoorSaveFields &fields,
                                             GameTitle title) {
    requireMM6(title);
    const auto &mapName = fields.raw.metadata.header.mapFileName;
    requireMapName(mapName);
    if (fields.generatedNpcs.mapName != mapName || fields.actors.mapName != mapName ||
        fields.sprites.mapName != mapName || fields.decorations.mapName != mapName ||
        fields.runtimeDecorations.mapName != mapName ||
        fields.runtimeSprites.mapName != mapName ||
        fields.runtimeActors.mapName != mapName)
        throw std::runtime_error("MM6 v6 indoor owners differ from HEAD map");
    SaveContainer container = decodeSaveContainer(encodeRawSave(fields.raw, title));
    container.version = 6;
    container.sections.push_back(encodeGeneratedNpcSection(fields.generatedNpcs, title));
    container.sections.push_back(encodeGameplayRngSection(fields.gameplayRng, title));
    container.sections.push_back(encodeEventRuntimeSection(fields.eventRuntime, title));
    if (!fields.faceOverrides.maps.empty())
        container.sections.push_back(encodeIndoorFaceOverrideSection(fields.faceOverrides));
    container.sections.push_back(encodeAnchor(fields.actors, IACR, 548, MAX_ACTORS));
    container.sections.push_back(encodeAnchor(fields.sprites, ISCR, 100, MAX_SPRITES));
    container.sections.push_back(encodeAnchor(fields.decorations, IDCR, 60, MAX_DECORATIONS));
    container.sections.push_back(encodeIndoorDecorationRuntimeSection(
        fields.runtimeDecorations, title));
    requireIndoorSpriteRuntimeSourceAgreement(
        fields.runtimeSprites, mapName, fields.sprites, title);
    container.sections.push_back(encodeSpriteRuntimeSectionAs(
        fields.runtimeSprites, SpriteRuntimeSectionKind::IndoorV6, title));
    requireIndoorActorRuntimeSourceAgreement(
        fields.runtimeActors, mapName, fields.actors, title);
    container.sections.push_back(encodeActorRuntimeSectionAs(
        fields.runtimeActors, ActorRuntimeSectionKind::IndoorV6, title));
    requireManifest(container);
    return encodeSaveContainer(container);
}

V6IndoorSaveFields decodeV6IndoorSave(std::span<const std::uint8_t> bytes,
                                     GameTitle title) {
    requireMM6(title);
    SaveContainer container = decodeSaveContainer(bytes);
    requireManifest(container);
    V6IndoorSaveFields result;
    result.generatedNpcs = decodeGeneratedNpcSection(container, title);
    result.gameplayRng = decodeGameplayRngSection(container, title);
    result.eventRuntime = decodeEventRuntimeSection(container, title);
    result.faceOverrides = decodeIndoorFaceOverrideSection(container);
    result.actors = decodeAnchor(container, IACR, 548, MAX_ACTORS);
    result.sprites = decodeAnchor(container, ISCR, 100, MAX_SPRITES);
    result.decorations = decodeAnchor(container, IDCR, 60, MAX_DECORATIONS);
    result.runtimeDecorations = decodeIndoorDecorationRuntimeSection(container, title);
    result.runtimeSprites = decodeSpriteRuntimeSectionAs(
        container, SpriteRuntimeSectionKind::IndoorV6, title);
    result.runtimeActors = decodeActorRuntimeSectionAs(
        container, ActorRuntimeSectionKind::IndoorV6, title);
    container.version = 2;
    std::erase_if(container.sections, [](const SaveSection &section) {
        return isNewTag(section.tag);
    });
    result.raw = decodeRawSave(encodeSaveContainer(container), title);
    if (encodeV6IndoorSave(result, title) !=
        std::vector<std::uint8_t>(bytes.begin(), bytes.end()))
        throw std::runtime_error("MM6 v6 indoor save is not canonical");
    return result;
}

StagedV6IndoorSaveFields stageV6IndoorSave(
    std::span<const std::uint8_t> bytes, const NpcStaticBaseline &freshNpcs,
    std::span<const std::string> expectedNames, const MapBaseReader &readBase,
    const GroundItemVerifier &verifyItem, const ItemTable &itemTable,
    const DecorationTable &decorationTable,
    std::size_t overlayTableCount, std::size_t tileTableCount,
    const IndoorBaseFaceReader &readPreEventFaces, GameTitle title) {
    auto saved = std::unique_ptr<V6IndoorSaveFields>(
        new V6IndoorSaveFields(decodeV6IndoorSave(bytes, title)));
    const auto rawBytes = encodeRawSave(saved->raw, title);
    auto represented = std::unique_ptr<StagedRawSessionFields>(
        new StagedRawSessionFields(stageRawSessionFields(
        rawBytes, freshNpcs, expectedNames, readBase, verifyItem, itemTable,
        overlayTableCount, tileTableCount, title)));
    requireNativeJoin(*saved, *represented);
    const auto active = std::find_if(saved->eventRuntime.maps.begin(),
                                    saved->eventRuntime.maps.end(),
        [&represented](const EventRuntimeMapEntry &entry) {
            return entry.mapName == represented->indoorPlan->mapName;
        });
    if (active == saved->eventRuntime.maps.end())
        throw std::runtime_error("MM6 v6 IDRT lacks active EVRT owner");
    auto decorationReplay = stageIndoorDecorationRuntime(
        saved->runtimeDecorations, represented->indoorPlan->base,
        *active, decorationTable, title);
    auto spriteReplay = stageIndoorSpriteRuntime(
        saved->runtimeSprites, represented->indoorPlan->mapName,
        saved->sprites, title);
    auto actorReplay = stageIndoorActorRuntime(
        saved->runtimeActors, represented->indoorPlan->mapName,
        saved->actors, title);
    std::deque<Actor> actorLinks(actorReplay.begin(), actorReplay.end());
    try {
        requireGeneratedNpcActorAgreement(
            saved->generatedNpcs, actorLinks, represented->indoorPlan->mapName, title);
    } catch (const std::exception &error) {
        throw std::runtime_error("MM6 v6 IART detached GNPC join: " +
                                 std::string(error.what()));
    }
    if (!readPreEventFaces)
        throw std::runtime_error("MM6 v6 indoor IFOV lacks pre-event face provenance");
    const auto baseline = readPreEventFaces(represented->indoorPlan->mapName);
    if (baseline.size() != represented->indoorPlan->base.faces.size())
        throw std::runtime_error("MM6 v6 indoor IFOV baseline differs from BLV face count");
    (void) stageIndoorFaceOverrides(saved->faceOverrides, readPreEventFaces, title);
    return {std::move(*saved), std::move(*represented),
            std::move(decorationReplay), std::move(spriteReplay),
            std::move(actorReplay)};
}

} // namespace mm6
