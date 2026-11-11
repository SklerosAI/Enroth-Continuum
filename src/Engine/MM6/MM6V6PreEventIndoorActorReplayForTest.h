#pragma once

#include <array>
#include <cstddef>
#include <deque>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"

#include "MM6GeneratedNpcSaveFields.h"
#include "MM6NativeActorNpcPolicy.h"
#include "MM6PreEventCheckpointForTest.h"
#include "MM6PreEventGeneratedNpcReplay.h"
#include "MM6V6IndoorSaveFields.h"

namespace mm6::testing {

class ScopedV6PreEventIndoorActorReplayForTest;

namespace detail {
inline ScopedV6PreEventIndoorActorReplayForTest *&activeV6IndoorActorReplayScope() {
    static thread_local ScopedV6PreEventIndoorActorReplayForTest *active = nullptr;
    return active;
}
} // namespace detail

// Test-only IART owner. Native IACR actor identities and GNPC links are
// validated before replacing the complete live deque. It is armed only for a
// disposable indoor reload after a strict v6 stage.
class ScopedV6PreEventIndoorActorReplayForTest {
 public:
    ScopedV6PreEventIndoorActorReplayForTest(
        GameTitle title, ActorRuntimeSaveFields fields,
        IndoorNativeSlotAnchor source, GeneratedNpcSaveFields generated,
        GameplayRandomState rng, bool publishGenerated)
        : _fields(std::move(fields)), _source(std::move(source)),
          _generated(std::move(generated)), _rng(std::move(rng)),
          _publishGenerated(publishGenerated) {
        if (title != GameTitle::MM6 ||
            detail::activeV6IndoorActorReplayScope())
            throw std::runtime_error("MM6 v6 IART requires a sole title-local replay scope");
        requireIndoorActorRuntimeSourceAgreement(
            _fields, _fields.mapName, _source, title);
        const std::deque<Actor> actors(_fields.actors.begin(), _fields.actors.end());
        try {
            requireGeneratedNpcActorAgreement(
                _generated, actors, _fields.mapName, title);
        } catch (const std::exception &error) {
            throw std::runtime_error("MM6 v6 IART scoped GNPC join: " +
                                     std::string(error.what()));
        }
        detail::activeV6IndoorActorReplayScope() = this;
    }

    ScopedV6PreEventIndoorActorReplayForTest(
        const ScopedV6PreEventIndoorActorReplayForTest &) = delete;
    ScopedV6PreEventIndoorActorReplayForTest &operator=(
        const ScopedV6PreEventIndoorActorReplayForTest &) = delete;

    ~ScopedV6PreEventIndoorActorReplayForTest() {
        if (detail::activeV6IndoorActorReplayScope() == this)
            detail::activeV6IndoorActorReplayScope() = nullptr;
    }

    bool published() const { return _published; }
    bool generatedBypassConsumed() const { return _bypassConsumed; }
    bool rngPublished() const { return _rngPublished; }

 private:
    friend bool runArmedV6PreEventIndoorActorReplayForTest(
        PreEventCheckpointResult, const std::deque<Actor> &,
        std::deque<Actor> *, NPCStats *, RandomEngineType, bool,
        std::string_view, GameTitle);
    friend bool consumeV6IndoorGeneratedNpcBypassForTest(GameTitle);
    friend std::vector<std::size_t> nativeClearedIndoorActorSlotsForTest(GameTitle);
    friend void restoreV6IndoorRngBeforeMapLoadForTest(
        const std::function<void(const GameplayRandomState &)> &, GameTitle);
    friend void requireV6IndoorReplayBeforeMapLoadForTest(GameTitle);

    ActorRuntimeSaveFields _fields;
    IndoorNativeSlotAnchor _source;
    GeneratedNpcSaveFields _generated;
    GameplayRandomState _rng;
    bool _publishGenerated = false;
    bool _pending = true;
    bool _published = false;
    bool _bypassConsumed = false;
    bool _rngPublished = false;
};

inline bool runArmedV6PreEventIndoorActorReplayForTest(
    PreEventCheckpointResult checkpoint,
    const std::deque<Actor> &rebuilt, std::deque<Actor> *destination,
    NPCStats *liveNpcs, RandomEngineType selectedRngType,
    bool eventRuntimeRestored, std::string_view loadedMapName,
    GameTitle title) {
    auto *scope = detail::activeV6IndoorActorReplayScope();
    if (!scope)
        return false;
    if (title != GameTitle::MM6 || checkpoint != PreEventCheckpointResult::Continue ||
        !scope->_pending || !eventRuntimeRestored || !destination || !liveNpcs ||
        destination != &rebuilt || loadedMapName != scope->_fields.mapName)
        throw std::runtime_error("MM6 v6 IART lacks exact pre-event context");
    std::vector<Actor> detached(rebuilt.begin(), rebuilt.end());
    applyIndoorActorRuntimeToDetachedList(
        scope->_fields, loadedMapName, scope->_source, &detached, title);
    std::deque<Actor> desired(detached.begin(), detached.end());
    try {
        requireGeneratedNpcActorAgreement(
            scope->_generated, desired, loadedMapName, title);
    } catch (const std::exception &error) {
        throw std::runtime_error("MM6 v6 IART pre-event GNPC join: " +
                                 std::string(error.what()));
    }
    static_assert(std::is_nothrow_swappable_v<std::deque<Actor>>);
    static_assert(std::is_nothrow_swappable_v<std::array<NPCData, 100>>);
    if (scope->_publishGenerated) {
        // IART holds the post-map value, while GNPC's pre-event row plan must
        // see the original source marker. Rebuild that join in a
        // detached copy. The published actor remains cleared.
        std::deque<Actor> nativeJoin = desired;
        constexpr std::size_t nativeWidth = 548;
        for (std::size_t slot = 0; slot < scope->_source.count; ++slot) {
            const auto native = projectMM6ActorRecord(
                std::span(scope->_source.records).subspan(slot * nativeWidth, nativeWidth));
            if (clearsNativeActorNpc(
                    static_cast<MonsterId>(native.monsterInfo.id), native.npcId) &&
                desired[slot].npcId == 0)
                nativeJoin[slot].npcId = native.npcId;
        }
        const auto generated = stagePreEventGeneratedNpcReplay(
            scope->_generated, scope->_rng, nativeJoin, *liveNpcs,
            loadedMapName, selectedRngType, title);
        auto rows = liveNpcs->pAdditionalNPC;
        for (std::size_t index = 0; index < generated.generatedCount; ++index)
            rows[index] = generated.generated.rows[index].npc;
        liveNpcs->pAdditionalNPC.swap(rows);
        liveNpcs->uNewlNPCBufPos = static_cast<int>(generated.generatedCount);
    }
    destination->swap(desired);
    scope->_pending = false;
    scope->_published = true;
    return true;
}

inline bool consumeV6IndoorGeneratedNpcBypassForTest(GameTitle title) {
    auto *scope = detail::activeV6IndoorActorReplayScope();
    if (!scope)
        return false;
    if (title != GameTitle::MM6 || !scope->_published || scope->_bypassConsumed)
        throw std::runtime_error("MM6 v6 indoor GNPC bypass lacks one published owner");
    if (!scope->_publishGenerated)
        return false;
    scope->_bypassConsumed = true;
    return true;
}

inline std::vector<std::size_t> nativeClearedIndoorActorSlotsForTest(GameTitle title) {
    auto *scope = detail::activeV6IndoorActorReplayScope();
    if (!scope)
        return {};
    if (title != GameTitle::MM6 || !scope->_published || !scope->_bypassConsumed)
        throw std::runtime_error("MM6 native-cleared actor slots lack replay context");
    constexpr std::size_t nativeWidth = 548;
    std::vector<std::size_t> slots;
    for (std::size_t slot = 0; slot < scope->_source.count; ++slot) {
        const auto native = projectMM6ActorRecord(
            std::span(scope->_source.records).subspan(slot * nativeWidth, nativeWidth));
        if (clearsNativeActorNpc(
                static_cast<MonsterId>(native.monsterInfo.id), native.npcId)) {
            if (scope->_fields.actors[slot].npcId != 0)
                throw std::runtime_error("MM6 IART retained a native-cleared NPC marker");
            slots.push_back(slot);
        }
    }
    return slots;
}

inline void restoreV6IndoorRngBeforeMapLoadForTest(
    const std::function<void(const GameplayRandomState &)> &restore,
    GameTitle title) {
    auto *scope = detail::activeV6IndoorActorReplayScope();
    if (!scope)
        return;
    if (!scope->_publishGenerated)
        return;
    if (title != GameTitle::MM6 || !scope->_published ||
        !scope->_bypassConsumed || scope->_rngPublished || !restore)
        throw std::runtime_error("MM6 v6 indoor RNG restore lacks a published GNPC owner");
    restore(scope->_rng);
    scope->_rngPublished = true;
}

inline void requireV6IndoorReplayBeforeMapLoadForTest(GameTitle title) {
    auto *scope = detail::activeV6IndoorActorReplayScope();
    if (scope && scope->_publishGenerated &&
        (title != GameTitle::MM6 || !scope->_published ||
                  !scope->_bypassConsumed || !scope->_rngPublished))
        throw std::runtime_error("MM6 v6 indoor replay is incomplete before map reload");
}

} // namespace mm6::testing
