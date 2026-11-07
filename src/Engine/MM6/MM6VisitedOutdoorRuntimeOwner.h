#pragma once

#include <algorithm>
#include <deque>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "MM6ChestSaveFields.h"
#include "MM6EventRuntimeSaveFields.h"
#include "MM6GeneratedNpcSaveFields.h"
#include "MM6MapSaveFields.h"
#include "MM6V5FullSessionPreflight.h"
#include "MM6V5OutdoorMapIdentity.h"

namespace mm6 {

// A detached, source-backed old outdoor map owner. It is intentionally not a
// save section: EC6 v5 has only one active-map ACTR/ARTR/SPRT/DECR/CHST set.
// The owning v5 preflight must have succeeded before this is constructed.
struct VisitedOutdoorRuntimeOwner {
    std::string mapName;
    SavedMapDelta nativeDelta;
    std::vector<std::uint8_t> actorPayload;
    GeneratedNpcSaveFields generatedNpcs;
    ChestSaveFields chest;
    EventRuntimeMapEntry event;
    ActorRuntimeSaveFields actors;
    SpriteRuntimeSaveFields sprites;
    DecorationRuntimeSaveFields decorations;

    [[nodiscard]] bool canWriteCheckpoint() const { return false; }
};

// Structural gate for a detached wire owner. Source-base agreement is checked
// separately by requireVisitedOutdoorRuntimeOwner before the owner is used.
inline void requireVisitedOutdoorRuntimeWireOwner(
    const VisitedOutdoorRuntimeOwner &owner, std::string_view expectedMap,
    GameTitle title) {
    if (title != GameTitle::MM6 || !isMM6OutdoorMapName(expectedMap) ||
        owner.mapName != expectedMap)
        throw std::runtime_error("MM6 visited outdoor map identity differs");
    std::string deltaName(expectedMap);
    deltaName.replace(deltaName.size() - 4, 4, ".ddm");
    if (owner.nativeDelta.name != deltaName ||
        owner.chest.activeDeltaName != deltaName ||
        owner.nativeDelta.bytes.empty() ||
        owner.nativeDelta.bytes != owner.chest.rawActiveDelta ||
        owner.generatedNpcs.mapName != expectedMap ||
        owner.event.mapName != expectedMap ||
        owner.actors.mapName != expectedMap ||
        owner.sprites.mapName != expectedMap ||
        owner.decorations.mapName != expectedMap)
        throw std::runtime_error("MM6 visited outdoor owners lack exact map join");
    requireActorRuntimeBaselineAgreement(owner.actors, expectedMap,
                                         owner.actorPayload, title);
    requireSpriteRuntimeNativeAgreement(owner.sprites, expectedMap,
                                        owner.nativeDelta.bytes, title);
    const std::deque<Actor> orderedActors(owner.actors.actors.begin(),
                                          owner.actors.actors.end());
    requireGeneratedNpcActorAgreement(owner.generatedNpcs, orderedActors,
                                      expectedMap, title);
}

// Recheck the joined owners against the same loaded source archive before a
// transition and after detaching. A stale native map cache, changed source,
// swapped map name or missing runtime owner refuses without touching live state.
inline void requireVisitedOutdoorRuntimeOwner(
    const VisitedOutdoorRuntimeOwner &owner, std::string_view expectedMap,
    const Blob &decodedBase, const DecorationTable &decorationTable,
    GameTitle title) {
    requireVisitedOutdoorRuntimeWireOwner(owner, expectedMap, title);
    requireDecorationRuntimeBaseAgreement(owner.decorations, expectedMap,
                                          decodedBase, owner.event,
                                          decorationTable, title);
}

inline VisitedOutdoorRuntimeOwner stageVisitedOutdoorRuntimeOwner(
    const V5FullSessionPreflight &source, std::string_view expectedMap,
    const Blob &decodedBase, const DecorationTable &decorationTable,
    GameTitle title) {
    if (title != GameTitle::MM6 ||
        source.v4.v3.actorMap.activeMap.mapName != expectedMap ||
        source.v4.saved.v3.raw.metadata.header.mapFileName != expectedMap ||
        source.v4.saved.eventRuntime.maps.size() != 1 ||
        source.v4.activeEventRuntime != source.v4.saved.eventRuntime.maps.front())
        throw std::runtime_error("MM6 visited outdoor source preflight differs");
    std::string deltaName(expectedMap);
    deltaName.replace(deltaName.size() - 4, 4, ".ddm");
    const auto &deltas = source.v4.saved.v3.raw.maps.deltas;
    const auto it = std::find_if(deltas.begin(), deltas.end(), [&](const SavedMapDelta &entry) {
        return entry.name == deltaName;
    });
    if (it == deltas.end())
        throw std::runtime_error("MM6 visited outdoor source delta is absent");
    VisitedOutdoorRuntimeOwner owner{
        std::string(expectedMap), *it,
        source.v4.saved.v3.activeActorPayload,
        source.v4.saved.v3.generatedNpcs,
        source.v4.saved.v3.raw.chest,
        source.v4.activeEventRuntime,
        source.actors, source.sprites, source.decorations};
    requireVisitedOutdoorRuntimeOwner(owner, expectedMap, decodedBase,
                                      decorationTable, title);
    return owner;
}

} // namespace mm6
