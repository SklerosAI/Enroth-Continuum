#pragma once

#include <algorithm>
#include <cstddef>
#include <deque>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "Engine/Snapshots/MM6IndoorSnapshot.h"
#include "Utility/Memory/Blob.h"

#include "MM6ChestSaveFields.h"
#include "MM6EventRuntimeSaveFields.h"
#include "MM6GeneratedNpcSaveFields.h"
#include "MM6IndoorFaceOverrides.h"
#include "MM6MapSaveFields.h"
#include "MM6V6IndoorSaveFields.h"

namespace mm6 {

inline constexpr std::size_t MM6_INDOOR_ACTOR_RECORD_BYTES = 548;
inline constexpr std::size_t MM6_INDOOR_GROUND_ITEM_RECORD_BYTES = 100;
inline constexpr std::size_t MM6_INDOOR_DECORATION_RECORD_BYTES = 60;

/**
 * Detached runtime owners of an indoor map the party has left, the indoor counterpart of `VisitedOutdoorRuntimeOwner`.
 * `nativeActors` and `nativeSprites` are the DLV actor and ground item records, `nativeDecorations` the BLV ones.
 */
struct VisitedIndoorRuntimeOwner {
    std::string mapName;
    SavedMapDelta nativeDelta;
    GeneratedNpcSaveFields generatedNpcs;
    ChestSaveFields chest;
    EventRuntimeMapEntry event;
    std::optional<IndoorFaceMapOverrides> faceOverrides;
    IndoorNativeSlotAnchor nativeActors;
    IndoorNativeSlotAnchor nativeSprites;
    IndoorNativeSlotAnchor nativeDecorations;
    IndoorDecorationRuntimeFields decorations;
    SpriteRuntimeSaveFields sprites;
    ActorRuntimeSaveFields actors;

    [[nodiscard]] bool canWriteCheckpoint() const { return false; }
};

/**
 * @param name                          Map file name.
 * @return                              Whether `name` is a canonical lower-case `.blv` name, as the v6 indoor owners
 *                                      spell it.
 */
[[nodiscard]] inline bool isMM6IndoorMapName(std::string_view name) {
    return name.size() > 4 && name.size() <= 63 && name.ends_with(".blv") &&
        std::all_of(name.begin(), name.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.';
        });
}

/**
 * Structural check of a detached owner, without the source map. Throws on a missing or mismatched owner.
 *
 * @param owner                         Owner to check.
 * @param expectedMap                   Indoor map the owner must belong to.
 * @param title                         Game title, only MM6 is accepted.
 */
inline void requireVisitedIndoorRuntimeWireOwner(const VisitedIndoorRuntimeOwner &owner, std::string_view expectedMap,
                                                 GameTitle title) {
    if (title != GameTitle::MM6 || !isMM6IndoorMapName(expectedMap) || owner.mapName != expectedMap)
        throw std::runtime_error("MM6 visited indoor map identity differs");
    std::string deltaName(expectedMap);
    deltaName.replace(deltaName.size() - 4, 4, ".dlv");
    const auto anchorJoins = [&](const IndoorNativeSlotAnchor &anchor, std::size_t width) {
        return anchor.mapName == expectedMap && anchor.records.size() == anchor.count * width;
    };
    if (owner.nativeDelta.name != deltaName || owner.chest.activeDeltaName != deltaName ||
        owner.nativeDelta.bytes.empty() || owner.nativeDelta.bytes != owner.chest.rawActiveDelta ||
        owner.generatedNpcs.mapName != expectedMap || owner.event.mapName != expectedMap ||
        (owner.faceOverrides && owner.faceOverrides->mapName != expectedMap) ||
        !anchorJoins(owner.nativeActors, MM6_INDOOR_ACTOR_RECORD_BYTES) ||
        !anchorJoins(owner.nativeSprites, MM6_INDOOR_GROUND_ITEM_RECORD_BYTES) ||
        !anchorJoins(owner.nativeDecorations, MM6_INDOOR_DECORATION_RECORD_BYTES) ||
        owner.decorations.mapName != expectedMap || owner.decorations.records.size() != owner.nativeDecorations.count ||
        owner.sprites.mapName != expectedMap || owner.actors.mapName != expectedMap)
        throw std::runtime_error("MM6 visited indoor owners lack exact map join");
    requireIndoorSpriteRuntimeSourceAgreement(owner.sprites, expectedMap, owner.nativeSprites, title);
    requireIndoorActorRuntimeSourceAgreement(owner.actors, expectedMap, owner.nativeActors, title);
    const std::vector<Actor> staged = stageIndoorActorRuntime(owner.actors, expectedMap, owner.nativeActors, title);
    const std::deque<Actor> orderedActors(staged.begin(), staged.end());
    requireGeneratedNpcActorAgreement(owner.generatedNpcs, orderedActors, expectedMap, title);
}

/**
 * Full check of a detached owner against the loaded source BLV. Throws if the native delta, the slot anchors,
 * the decoration runtime or the face overrides disagree with that base.
 *
 * @param owner                         Owner to check.
 * @param expectedMap                   Indoor map the owner must belong to.
 * @param base                          Decoded BLV of `expectedMap` from the loaded archive.
 * @param decorationTable               Loaded decoration table.
 * @param readBaseFaces                 Pre-event face reader, only called when the owner has face overrides.
 * @param title                         Game title, only MM6 is accepted.
 */
inline void requireVisitedIndoorRuntimeOwner(const VisitedIndoorRuntimeOwner &owner, std::string_view expectedMap,
                                             const MM6IndoorSnapshot &base, const DecorationTable &decorationTable,
                                             const IndoorBaseFaceReader &readBaseFaces, GameTitle title) {
    requireVisitedIndoorRuntimeWireOwner(owner, expectedMap, title);
    const MM6IndoorRuntimeDelta delta = decodeMM6IndoorRuntimeDelta(
        Blob::view(owner.nativeDelta.bytes.data(), owner.nativeDelta.bytes.size()), base);
    const auto sameRecords = [](const IndoorNativeSlotAnchor &anchor, const MM6IndoorSection &section) {
        const auto *records = static_cast<const std::uint8_t *>(section.bytes.data()) + 4;
        return section.bytes.size() == 4 + anchor.records.size() && section.count == anchor.count &&
            std::equal(anchor.records.begin(), anchor.records.end(), records);
    };
    if (!sameRecords(owner.nativeActors, delta.wire.section(MM6IndoorSectionKind::Actors)) ||
        !sameRecords(owner.nativeSprites, delta.wire.section(MM6IndoorSectionKind::GroundItems)) ||
        !sameRecords(owner.nativeDecorations, base.section(MM6IndoorSectionKind::Sprites)))
        throw std::runtime_error("MM6 visited indoor native slot anchor differs from BLV/DLV");
    if (owner.event.respawnCount != delta.respawnCount || owner.event.lastRespawnDay != delta.lastRespawnDay)
        throw std::runtime_error("MM6 visited indoor EVRT differs from DLV reset header");
    const IndoorLocation_MM7 projected = projectMM6IndoorBase(base);
    requireIndoorDecorationRuntimeSourceAgreement(owner.decorations, expectedMap, projected, owner.event,
                                                  decorationTable, title);
    if (owner.faceOverrides)
        (void) stageIndoorFaceOverrides({{*owner.faceOverrides}}, readBaseFaces, title);
}

/**
 * Detaches the active map of an already staged v6 indoor save into a visited owner.
 *
 * @param source                        Staged v6 indoor save whose active map is `expectedMap`.
 * @param expectedMap                   Indoor map to detach.
 * @param decorationTable               Loaded decoration table.
 * @param readBaseFaces                 Pre-event face reader for `expectedMap`.
 * @param title                         Game title, only MM6 is accepted.
 * @return                              Owner that passed `requireVisitedIndoorRuntimeOwner`.
 */
[[nodiscard]] inline VisitedIndoorRuntimeOwner stageVisitedIndoorRuntimeOwner(
    const StagedV6IndoorSaveFields &source, std::string_view expectedMap, const DecorationTable &decorationTable,
    const IndoorBaseFaceReader &readBaseFaces, GameTitle title) {
    const auto *active = std::get_if<StagedIndoorMap>(&source.represented.raw.activeMap);
    if (title != GameTitle::MM6 || !active || active->mapName != expectedMap ||
        source.saved.raw.metadata.header.mapFileName != expectedMap)
        throw std::runtime_error("MM6 visited indoor source differs");
    const auto &events = source.saved.eventRuntime.maps;
    const auto event = std::find_if(events.begin(), events.end(), [&](const EventRuntimeMapEntry &entry) {
        return entry.mapName == expectedMap;
    });
    if (event == events.end())
        throw std::runtime_error("MM6 visited indoor source lacks EVRT owner");
    const auto &faceMaps = source.saved.faceOverrides.maps;
    const auto faces = std::find_if(faceMaps.begin(), faceMaps.end(), [&](const IndoorFaceMapOverrides &entry) {
        return entry.mapName == expectedMap;
    });
    const ChestSaveFields &chest = source.saved.raw.chest;
    VisitedIndoorRuntimeOwner owner{
        std::string(expectedMap), {chest.activeDeltaName, chest.rawActiveDelta},
        source.saved.generatedNpcs, chest, *event,
        faces == faceMaps.end() ? std::nullopt : std::optional(*faces),
        source.saved.actors, source.saved.sprites, source.saved.decorations,
        source.saved.runtimeDecorations, source.saved.runtimeSprites, source.saved.runtimeActors};
    requireVisitedIndoorRuntimeOwner(owner, expectedMap, active->base, decorationTable, readBaseFaces, title);
    return owner;
}

} // namespace mm6
