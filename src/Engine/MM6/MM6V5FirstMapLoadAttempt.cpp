#include "MM6V5FirstMapLoadAttempt.h"

#include <algorithm>
#include <deque>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "Library/Platform/Application/PlatformApplication.h"

#include "MM6ChestLiveOwnerAdapter.h"
#include "MM6ChestSeedAndOwnership.h"
#include "MM6EventRuntimeSaveFields.h"
#include "MM6PreEventActorReplay.h"
#include "MM6V5ActorRuntimeFields.h"
#include "MM6V5FirstMapLoadPreflight.h"
#include "MM6V5FullSessionPreflight.h"

namespace mm6 {
namespace {

constexpr std::string_view FIRST_MAP = "oute3.odm";
constexpr std::string_view TEST_SLOT = "candidate-v5-after-action.ec6";

} // namespace

V5FirstMapLoadAttempt::V5FirstMapLoadAttempt(
    std::unique_ptr<V5FullSessionPreflight> staged,
    std::string_view slotName, GameTitle title)
    : _staged(std::move(staged)) {
    if (title != GameTitle::MM6 || slotName != TEST_SLOT || !_staged ||
        !engine || engine->gameTitle() != title)
        throw std::runtime_error("MM6 first-map load attempt lacks exact test slot/title");
    requireV5FirstMapPrepublicationAgreement(*_staged, title);
    const auto &actorMap = _staged->v4.v3.actorMap;
    if (actorMap.activeMap.mapName != FIRST_MAP || _staged->actors.mapName != FIRST_MAP ||
        actorMap.saved.activeActorPayload != _staged->v4.saved.v3.activeActorPayload)
        throw std::runtime_error("MM6 first-map load attempt actor owners differ from HEAD");
    requirePreEventActorReplaySource(actorMap, title);
    requireActorRuntimeBaselineAgreement(_staged->actors, FIRST_MAP,
                                         _staged->v4.saved.v3.activeActorPayload, title);
}

V5FirstMapLoadAttempt::~V5FirstMapLoadAttempt() = default;

V5FullSessionPreflight &V5FirstMapLoadAttempt::staged() { return *_staged; }
const V5FullSessionPreflight &V5FirstMapLoadAttempt::staged() const { return *_staged; }

void V5FirstMapLoadAttempt::notePublished(GameTitle title) {
    if (title != GameTitle::MM6 || _phase != Phase::Detached || !engine ||
        !pMapTable ||
        engine->gameTitle() != title || !engine->_pendingTransition ||
        engine->_pendingTransition->map() != pMapTable->GetMapInfo(FIRST_MAP))
        throw std::runtime_error("MM6 first-map publication has another phase or map");
    _phase = Phase::Published;
}

void V5FirstMapLoadAttempt::restoreEventAtNativeMap(
    std::string_view mapName, int respawnCount, int lastRespawnDay,
    bool wasRespawned, PersistentVariables *destination,
    std::span<LevelDecoration> decorations, GameTitle title) {
    if (title != GameTitle::MM6 || _phase != Phase::Published ||
        mapName != FIRST_MAP || wasRespawned || !destination ||
        respawnCount < 0 || lastRespawnDay <= 0 || !pOutdoor ||
        pOutdoor->mm6DeltaBytes !=
            _staged->v4.saved.v3.raw.chest.rawActiveDelta)
        throw std::runtime_error("MM6 first-map event restore lacks exact native owners");
    const auto &entry = requireEventRuntimeMapEntry(
        _staged->v4.saved.eventRuntime, mapName, respawnCount,
        lastRespawnDay, decorations, title);
    static_assert(std::is_nothrow_copy_assignable_v<PersistentVariables>);
    PersistentVariables updated = *destination;
    updated.mapVars = entry.mapVars;
    updated.decorVars = entry.decorVars;
    *destination = updated;
    _phase = Phase::EventRestored;
}

void V5FirstMapLoadAttempt::publishActorsAtNativeMap(GameTitle title) {
    if (title != GameTitle::MM6 || _phase != Phase::EventRestored ||
        !engine || engine->gameTitle() != title || !pMapTable || !pNPCStats ||
        engine->_currentLoadedMapId != pMapTable->GetMapInfo(FIRST_MAP) ||
        nextActorReuseScanStart != 0 || engine->config->debug.NoActors.value() ||
        !chestOwnersUnchanged())
        throw std::runtime_error("MM6 first-map actor replay lacks exact native owners");
    const auto &actorMap = _staged->v4.v3.actorMap;
    PreEventActorReplayPlan plan = stagePreEventActorReplay(
        actorMap.activeActors, actorMap.saved.generatedNpcs, actorMap.saved.gameplayRng,
        pActors, *pNPCStats, FIRST_MAP, actorMap.activeMap.delta.actors.size(),
        std::max(0, engine->config->gameplay.MaxActors.value()),
        application->component<EngineRandomComponent>()->type(), title);
    std::deque<Actor> runtime(_staged->actors.actors.begin(), _staged->actors.actors.end());
    plan.actors.swap(runtime);
    publishPreEventActorReplay(std::move(plan), &pActors, pNPCStats, title);
    _phase = Phase::ActorsPublished;
}

bool V5FirstMapLoadAttempt::applyChestAtNativeMap(GameTitle title) {
    if (title != GameTitle::MM6 || _phase != Phase::ActorsPublished ||
        !engine || engine->gameTitle() != title || !pMapTable || !pItemTable ||
        !pParty || engine->_currentLoadedMapId != pMapTable->GetMapInfo(FIRST_MAP) ||
        !chestOwnersUnchanged())
        throw std::runtime_error("MM6 first-map chest handoff lacks exact native owners");
    const auto owned = scanPartyOwnedChestArtifacts(title, *pParty);
    if (!owned || !applyActiveChestLiveOwnerHandoff(
            title, pMapTable->pInfos[engine->_currentLoadedMapId].mapTreasureLevel,
            *pItemTable, *owned, true))
        throw std::runtime_error("MM6 first-map chest handoff refused");
    _phase = Phase::ChestApplied;
    return true;
}

bool V5FirstMapLoadAttempt::keepGeneratedNpcsAtNativeMap(GameTitle title) {
    if (title != GameTitle::MM6 || _phase != Phase::ChestApplied)
        throw std::runtime_error("MM6 first-map generated NPC rows were not published with the actors");
    _phase = Phase::GeneratedNpcsKept;
    return true;
}

void V5FirstMapLoadAttempt::restoreRngBeforeEvents(GameTitle title) {
    if (title != GameTitle::MM6 || _phase != Phase::GeneratedNpcsKept ||
        !engine || engine->gameTitle() != title || !chestOwnersUnchanged())
        throw std::runtime_error("MM6 first-map RNGS restore lacks exact native owners");
    application->component<EngineRandomComponent>()->restoreGameplayState(
        _staged->v4.v3.actorMap.saved.gameplayRng);
    _phase = Phase::RngRestored;
}

bool V5FirstMapLoadAttempt::chestOwnersUnchanged() const {
    const auto &chest = _staged->v4.saved.v3.raw.chest;
    return pOutdoor && uCurrentlyLoadedLevelType == LEVEL_OUTDOOR &&
           pOutdoor->mm6DeltaBytes == chest.rawActiveDelta &&
           chestTitleState().ready &&
           chestTitleState().lcgState == chest.lcgState &&
           chestTitleState().generatedArtifactFlags == chest.generatedArtifactFlags;
}

} // namespace mm6
