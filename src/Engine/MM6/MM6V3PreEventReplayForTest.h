#pragma once

#include <array>
#include <cstddef>
#include <deque>
#include <functional>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "Engine/Components/Random/EngineRandomComponent.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Tables/NPCTable.h"

#include "MM6PreEventCheckpointForTest.h"
#include "MM6ChestTitleState.h"
#include "MM6V3OutdoorActorMapAgreement.h"

namespace mm6::testing {

struct V3PreEventReplayPlan {
    std::deque<Actor> actors;
    std::array<NPCData, 100> generatedRows;
    GameplayRandomState gameplayRng;
    int generatedCount = 0;
};

// Detached final pre-event agreement. ACTR is authoritative for mutable actor
// fields, while the rebuilt native DDM prefix must keep exact PID anchors.
// GNPC must cover every generated peasant in the complete ACTR list.
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
    GameTitle title);

// Commit only ACTR and GNPC before _461103. RNGS is restored later, after
// chest/arena/decoration/cursor setup and immediately before OnMapReload.
void publishV3PreEventReplayForTest(
    V3PreEventReplayPlan plan, std::deque<Actor> *liveActors,
    NPCStats *liveNpcs, GameTitle title);

class ScopedV3PreEventReplayForTest;

namespace detail {

inline ScopedV3PreEventReplayForTest *&activeV3PreEventReplayScope() {
    static thread_local ScopedV3PreEventReplayForTest *active = nullptr;
    return active;
}

} // namespace detail

// A disposable fresh-child test installs a fully staged v3 map agreement and
// the ordinary pre-event checkpoint scope. No production SaveLoad caller arms
// this context. Scope teardown clears unconsumed state.
class ScopedV3PreEventReplayForTest {
 public:
    ScopedV3PreEventReplayForTest(GameTitle title, V3OutdoorActorMapAgreement agreement);
    ScopedV3PreEventReplayForTest(const ScopedV3PreEventReplayForTest &) = delete;
    ScopedV3PreEventReplayForTest &operator=(const ScopedV3PreEventReplayForTest &) = delete;
    ~ScopedV3PreEventReplayForTest();

    bool published() const { return _published; }
    bool rngPublished() const { return _rngPublished; }

 private:
    friend bool runArmedV3PreEventReplayForTest(
        PreEventCheckpointResult checkpoint, const std::deque<Actor> &rebuiltActors,
        std::deque<Actor> *liveActors, NPCStats *liveNpcs,
        RandomEngineType selectedRngType, std::string_view loadedMapName,
        std::size_t runtimeActorCap, std::size_t actorReuseScanStart,
        bool noActorsDebug, bool outdoor, bool chestOptedIn,
        const ChestTitleState *chestOwner,
        std::span<const std::uint8_t> activeDelta, GameTitle title);
    friend bool consumeV3GeneratedNpcBypassForTest(GameTitle title);
    friend void restoreV3RngBeforeMapLoadForTest(
        const std::function<void(const GameplayRandomState &)> &restoreRng,
        const ChestTitleState *chestOwner,
        std::span<const std::uint8_t> activeDelta, GameTitle title);
    friend void requireV3ReplayBeforeMapLoadForTest(GameTitle title);
    V3OutdoorActorMapAgreement _agreement;
    bool _pending = true;
    bool _published = false;
    bool _bypassConsumed = false;
    bool _rngPublished = false;
};

// Called only at DoPrepareWorld's existing test checkpoint. Returns false
// when no v3 context is armed; an armed context fails closed on missing prior
// checkpoint, stale actors/NPCs, wrong map/RNG type, or incomplete sections.
bool runArmedV3PreEventReplayForTest(
    PreEventCheckpointResult checkpoint, const std::deque<Actor> &rebuiltActors,
    std::deque<Actor> *liveActors, NPCStats *liveNpcs,
    RandomEngineType selectedRngType, std::string_view loadedMapName,
    std::size_t runtimeActorCap, std::size_t actorReuseScanStart,
    bool noActorsDebug, bool outdoor, bool chestOptedIn,
    const ChestTitleState *chestOwner,
    std::span<const std::uint8_t> activeDelta, GameTitle title);

// Called at the generated-NPC loop only. The caller still runs chest, arena,
// decoration and cursor setup around that loop.
bool consumeV3GeneratedNpcBypassForTest(GameTitle title);

// Check that the saved CHST owner remained exact through chest handoff, then
// restore RNGS after all _461103 work and before onMapLoad events.
void restoreV3RngBeforeMapLoadForTest(
    const std::function<void(const GameplayRandomState &)> &restoreRng,
    const ChestTitleState *chestOwner,
    std::span<const std::uint8_t> activeDelta, GameTitle title);

// The replay context must have published and passed the narrow generated-NPC
// bypass before OnMapReload may execute.
void requireV3ReplayBeforeMapLoadForTest(GameTitle title);

} // namespace mm6::testing
