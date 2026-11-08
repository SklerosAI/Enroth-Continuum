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
 * Engine-owned first-map v5 load context. It owns the detached graph, replays EVRT, actors, generated NPCs,
 * decorations, sprites, the chest handoff and RNGS in native load order, and checks the prepared world before the
 * first input frame. It lives for one load. No normal SaveLoad route may construct this context yet.
 */
class V5FirstMapLoadAttempt {
 public:
    enum class Phase {
        Detached,
        Published,
        EventRestored,
        WorldPublished,
        ChestApplied,
        GeneratedNpcsKept,
        RngRestored,
        PostWorldAgreed
    };

    V5FirstMapLoadAttempt(std::unique_ptr<V5FullSessionPreflight> staged,
                          std::string_view slotName, GameTitle title);

    /**
     * Stages a first-map v5 slot and checks it before anything is published.
     *
     * @param slotBytes                 Validated primary bytes of the slot.
     * @param slotName                  A numbered `saveNNN.ec6` slot, or the disposable test candidate.
     * @param title                     Must be MM6.
     * @return                          Detached attempt. Throws `std::invalid_argument` for another slot name or
     *                                  title, and `std::runtime_error` when the slot can't be staged.
     */
    static std::unique_ptr<V5FirstMapLoadAttempt> fromNativeSlot(std::span<const std::uint8_t> slotBytes,
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
        return _phase >= Phase::EventRestored;
    }

    /**
     * Replaces the rebuilt native actors, generated NPC rows, decorations and sprites with the saved ARTR, GNPC, DECR
     * and SPRT lists. Every list is staged against its rebuilt native anchors and the unchanged chest owners before
     * any of them is swapped in. Called once, before `_461103_load_level_sub`.
     *
     * @param title                     Must be MM6.
     */
    void publishWorldAtNativeMap(GameTitle title);
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

    /**
     * Checks every represented owner of the prepared world against the staged graph. Called once, after map events
     * and before the first input frame. Throws on any difference.
     *
     * @param title                     Must be MM6.
     */
    void requirePostWorldAgreement(GameTitle title);

 private:
    bool chestOwnersUnchanged() const;

    std::unique_ptr<V5FullSessionPreflight> _staged;
    Phase _phase = Phase::Detached;
};

/**
 * Checks the world the engine's first-map load attempt prepared against its staged owners, then drops the attempt.
 * Called after world preparation and before the first input frame. A refused check abandons the load instead.
 *
 * @return                              Whether the prepared world was admitted.
 */
bool releaseV5FirstMapLoadAttempt();

/**
 * Drops the engine's failed first-map load attempt and sends the game loop back to the main menu before any input,
 * world update or save. This is not a rollback, owners published before the failure stay partial.
 *
 * @return                              Whether an attempt was in flight.
 */
bool abandonV5FirstMapLoadAttempt() noexcept;

} // namespace mm6
