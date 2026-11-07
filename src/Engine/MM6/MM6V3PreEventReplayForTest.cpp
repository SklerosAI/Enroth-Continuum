#include "MM6V3PreEventReplayForTest.h"

#include <algorithm>
#include <deque>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "MM6ActiveActorSection.h"
#include "MM6GameplayRngSaveFields.h"
#include "MM6GeneratedNpcSaveFields.h"
#include "MM6PreEventGeneratedNpcReplay.h"

namespace mm6::testing {
namespace {

bool sameNativeAnchor(const Actor &rebuilt, const Actor &saved, std::size_t slot) {
    return rebuilt.id == slot && saved.id == slot &&
           rebuilt.monsterInfo.id == saved.monsterInfo.id &&
           rebuilt.monsterId == saved.monsterId &&
           rebuilt.initialPosition == saved.initialPosition;
}

} // namespace

V3PreEventReplayPlan stageV3PreEventReplayForTest(
    const std::vector<Actor> &savedActors,
    const GeneratedNpcSaveFields &savedGenerated,
    const GameplayRandomState &savedRng,
    const std::deque<Actor> &rebuiltActors,
    const NPCStats &freshNpcs,
    std::string_view loadedMapName,
    std::size_t nativeActorCount,
    std::size_t runtimeActorCap,
    RandomEngineType selectedRngType,
    GameTitle title) {
    if (title != GameTitle::MM6 || loadedMapName.empty() ||
        rebuiltActors.size() != nativeActorCount ||
        savedActors.size() < nativeActorCount ||
        savedActors.size() > runtimeActorCap ||
        freshNpcs.uNewlNPCBufPos != 0)
        throw std::runtime_error("MM6 v3 pre-event replay lacks an exact fresh actor context");
    for (std::size_t slot = 0; slot < rebuiltActors.size(); ++slot)
        if (!sameNativeAnchor(rebuiltActors[slot], savedActors[slot], slot))
            throw std::runtime_error("MM6 v3 rebuilt actor anchor differs from ACTR");

    V3PreEventReplayPlan plan;
    plan.actors.assign(savedActors.begin(), savedActors.end());
    const auto generated = stagePreEventGeneratedNpcReplay(
        savedGenerated, savedRng, plan.actors, freshNpcs,
        loadedMapName, selectedRngType, title);
    requireGeneratedNpcActorAgreement(savedGenerated, plan.actors,
                                      loadedMapName, title);
    plan.generatedRows = freshNpcs.pAdditionalNPC;
    for (std::size_t row = 0; row < generated.generatedCount; ++row)
        plan.generatedRows[row] = generated.generated.rows[row].npc;
    plan.generatedCount = static_cast<int>(generated.generatedCount);
    plan.gameplayRng = generated.gameplayRng;
    return plan;
}

void publishV3PreEventReplayForTest(
    V3PreEventReplayPlan plan, std::deque<Actor> *liveActors,
    NPCStats *liveNpcs, GameTitle title) {
    if (title != GameTitle::MM6 || !liveActors || !liveNpcs)
        throw std::runtime_error("MM6 v3 pre-event replay lacks a publication owner");
    static_assert(std::is_nothrow_swappable_v<std::deque<Actor>>);
    static_assert(std::is_nothrow_swappable_v<std::array<NPCData, 100>>);
    liveActors->swap(plan.actors);
    liveNpcs->pAdditionalNPC.swap(plan.generatedRows);
    liveNpcs->uNewlNPCBufPos = plan.generatedCount;
}

ScopedV3PreEventReplayForTest::ScopedV3PreEventReplayForTest(
    GameTitle title, V3OutdoorActorMapAgreement agreement)
    : _agreement(std::move(agreement)) {
    if (title != GameTitle::MM6 || detail::activeV3PreEventReplayScope() ||
        _agreement.activeMap.mapName.empty() || _agreement.activeActors.empty() ||
        _agreement.saved.raw.metadata.header.mapFileName != _agreement.activeMap.mapName ||
        _agreement.saved.generatedNpcs.mapName != _agreement.activeMap.mapName ||
        _agreement.activeMap.delta.actors.size() != _agreement.activeSidecar.actors.size() ||
        encodeActiveActorSection(_agreement.activeActors, title) !=
            _agreement.saved.activeActorPayload)
        throw std::runtime_error("MM6 v3 pre-event test replay requires a sole staged map");
    (void) encodeGeneratedNpcSection(_agreement.saved.generatedNpcs, title);
    (void) encodeGameplayRngSection(_agreement.saved.gameplayRng, title);
    detail::activeV3PreEventReplayScope() = this;
}

ScopedV3PreEventReplayForTest::~ScopedV3PreEventReplayForTest() {
    if (detail::activeV3PreEventReplayScope() == this)
        detail::activeV3PreEventReplayScope() = nullptr;
}

bool runArmedV3PreEventReplayForTest(
    PreEventCheckpointResult checkpoint, const std::deque<Actor> &rebuiltActors,
    std::deque<Actor> *liveActors, NPCStats *liveNpcs,
    RandomEngineType selectedRngType, std::string_view loadedMapName,
    std::size_t runtimeActorCap, std::size_t actorReuseScanStart,
    bool noActorsDebug, bool outdoor, bool chestOptedIn,
    const ChestTitleState *chestOwner,
    std::span<const std::uint8_t> activeDelta, GameTitle title) {
    ScopedV3PreEventReplayForTest *scope = detail::activeV3PreEventReplayScope();
    if (!scope)
        return false;
    if (title != GameTitle::MM6 || checkpoint != PreEventCheckpointResult::Continue ||
        !scope->_pending || !liveActors || !liveNpcs ||
        liveActors != &rebuiltActors || actorReuseScanStart != 0 || noActorsDebug ||
        !outdoor || !chestOptedIn || !chestOwner || !chestOwner->ready ||
        loadedMapName != scope->_agreement.activeMap.mapName ||
        chestOwner->lcgState != scope->_agreement.saved.raw.chest.lcgState ||
        chestOwner->generatedArtifactFlags !=
            scope->_agreement.saved.raw.chest.generatedArtifactFlags ||
        !std::ranges::equal(activeDelta,
            scope->_agreement.saved.raw.chest.rawActiveDelta))
        throw std::runtime_error("MM6 v3 pre-event test replay has no safe publication point");
    scope->_pending = false;
    V3PreEventReplayPlan plan = stageV3PreEventReplayForTest(
        scope->_agreement.activeActors, scope->_agreement.saved.generatedNpcs,
        scope->_agreement.saved.gameplayRng, rebuiltActors, *liveNpcs,
        loadedMapName, scope->_agreement.activeMap.delta.actors.size(),
        runtimeActorCap, selectedRngType, title);

    // All validation/copies finish before the noexcept actor/NPC swaps.
    // RNGS remains staged until after _461103's other work.
    publishV3PreEventReplayForTest(std::move(plan), liveActors, liveNpcs, title);
    scope->_published = true;
    return true;
}

bool consumeV3GeneratedNpcBypassForTest(GameTitle title) {
    ScopedV3PreEventReplayForTest *scope = detail::activeV3PreEventReplayScope();
    if (!scope)
        return false;
    if (title != GameTitle::MM6 || !scope->_published || scope->_bypassConsumed)
        throw std::runtime_error("MM6 v3 generated NPC bypass lacks one published owner");
    scope->_bypassConsumed = true;
    return true;
}

void restoreV3RngBeforeMapLoadForTest(
    const std::function<void(const GameplayRandomState &)> &restoreRng,
    const ChestTitleState *chestOwner,
    std::span<const std::uint8_t> activeDelta, GameTitle title) {
    ScopedV3PreEventReplayForTest *scope = detail::activeV3PreEventReplayScope();
    if (!scope)
        return;
    if (title != GameTitle::MM6 || !scope->_published ||
        !scope->_bypassConsumed || scope->_rngPublished || !restoreRng ||
        !chestOwner || !chestOwner->ready ||
        chestOwner->lcgState != scope->_agreement.saved.raw.chest.lcgState ||
        chestOwner->generatedArtifactFlags !=
            scope->_agreement.saved.raw.chest.generatedArtifactFlags ||
        !std::ranges::equal(activeDelta,
            scope->_agreement.saved.raw.chest.rawActiveDelta))
        throw std::runtime_error("MM6 v3 chest owner changed before RNGS restoration");
    restoreRng(scope->_agreement.saved.gameplayRng);
    scope->_rngPublished = true;
}

void requireV3ReplayBeforeMapLoadForTest(GameTitle title) {
    ScopedV3PreEventReplayForTest *scope = detail::activeV3PreEventReplayScope();
    if (scope && (title != GameTitle::MM6 || !scope->_published ||
                  !scope->_bypassConsumed || !scope->_rngPublished))
        throw std::runtime_error("MM6 v3 pre-event replay did not precede map events");
}

} // namespace mm6::testing
