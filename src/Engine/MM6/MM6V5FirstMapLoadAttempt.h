#pragma once

#include <memory>
#include <span>
#include <string_view>

#include "Engine/GameTitle.h"

namespace mm6 {

struct V5FullSessionPreflight;

} // namespace mm6

struct PersistentVariables;
struct LevelDecoration;

namespace mm6 {

/**
 * Engine-owned first-map v5 load context. It owns the detached graph and replays EVRT, actors, generated NPCs, the
 * chest handoff and RNGS in native load order. Decoration and sprite replay still use disposable test hooks. No
 * normal SaveLoad route may construct this context yet.
 */
class V5FirstMapLoadAttempt {
 public:
    enum class Phase {
        Detached,
        Published,
        EventRestored,
        ActorsPublished,
        ChestApplied,
        GeneratedNpcsKept,
        RngRestored,
        Abandoned
    };

    V5FirstMapLoadAttempt(std::unique_ptr<V5FullSessionPreflight> staged,
                          std::string_view slotName, GameTitle title);
    ~V5FirstMapLoadAttempt();
    V5FirstMapLoadAttempt(const V5FirstMapLoadAttempt &) = delete;
    V5FirstMapLoadAttempt &operator=(const V5FirstMapLoadAttempt &) = delete;

    V5FullSessionPreflight &staged();
    const V5FullSessionPreflight &staged() const;
    Phase phase() const { return _phase; }
    void notePublished(GameTitle title);
    void restoreEventAtNativeMap(std::string_view mapName, int respawnCount,
                                 int lastRespawnDay, bool wasRespawned,
                                 PersistentVariables *destination,
                                 std::span<LevelDecoration> decorations,
                                 GameTitle title);
    bool eventRestored() const {
        return _phase >= Phase::EventRestored && _phase != Phase::Abandoned;
    }

    /**
     * Replaces the rebuilt native actors with the saved ARTR list and publishes the saved GNPC rows, after checking
     * the rebuilt DDM anchors, the ACTR baseline and the chest owners. Called once, before `_461103_load_level_sub`.
     *
     * @param title                     Must be MM6.
     */
    void publishActorsAtNativeMap(GameTitle title);
    bool applyChestAtNativeMap(GameTitle title);

    /**
     * @param title                     Must be MM6.
     * @return                          Always true. Generated NPC rows were published with the actors, so the
     *                                  native generation loop must be skipped. Throws out of phase.
     */
    bool keepGeneratedNpcsAtNativeMap(GameTitle title);

    /**
     * Restores the saved gameplay RNG state. Must be the last step before map events run.
     *
     * @param title                     Must be MM6.
     */
    void restoreRngBeforeEvents(GameTitle title);
    void abandon() noexcept { _phase = Phase::Abandoned; }

 private:
    bool chestOwnersUnchanged() const;

    std::unique_ptr<V5FullSessionPreflight> _staged;
    Phase _phase = Phase::Detached;
};

} // namespace mm6
