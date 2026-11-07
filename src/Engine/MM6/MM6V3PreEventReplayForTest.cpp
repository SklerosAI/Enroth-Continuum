#include "MM6V3PreEventReplayForTest.h"

#include <algorithm>
#include <deque>
#include <span>
#include <stdexcept>
#include <utility>

namespace mm6::testing {

ScopedV3PreEventReplayForTest::ScopedV3PreEventReplayForTest(
    GameTitle title, V3OutdoorActorMapAgreement agreement)
    : _agreement(std::move(agreement)) {
    if (title != GameTitle::MM6 || detail::activeV3PreEventReplayScope())
        throw std::runtime_error("MM6 v3 pre-event test replay requires a sole staged map");
    requirePreEventActorReplaySource(_agreement, title);
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
    PreEventActorReplayPlan plan = stagePreEventActorReplay(
        scope->_agreement.activeActors, scope->_agreement.saved.generatedNpcs,
        scope->_agreement.saved.gameplayRng, rebuiltActors, *liveNpcs,
        loadedMapName, scope->_agreement.activeMap.delta.actors.size(),
        runtimeActorCap, selectedRngType, title);

    // All validation/copies finish before the noexcept actor/NPC swaps.
    // RNGS remains staged until after _461103's other work.
    publishPreEventActorReplay(std::move(plan), liveActors, liveNpcs, title);
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
        !scope->_bypassConsumed || scope->_rngPublished || !restoreRng)
        throw std::runtime_error("MM6 v3 replay order changed before RNGS restoration");
    if (!chestOwner || !chestOwner->ready)
        throw std::runtime_error("MM6 v3 chest readiness changed before RNGS restoration");
    if (chestOwner->lcgState != scope->_agreement.saved.raw.chest.lcgState)
        throw std::runtime_error("MM6 v3 chest LCG changed before RNGS restoration");
    if (chestOwner->generatedArtifactFlags !=
        scope->_agreement.saved.raw.chest.generatedArtifactFlags)
        throw std::runtime_error("MM6 v3 chest artifact flags changed before RNGS restoration");
    if (!std::ranges::equal(activeDelta,
            scope->_agreement.saved.raw.chest.rawActiveDelta))
        throw std::runtime_error("MM6 v3 chest DDM changed before RNGS restoration");
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
