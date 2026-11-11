#pragma once

#include <functional>
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
 * first input frame. It lives for one load of a slot of the MM6 profile.
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

    /**
     * Stages a first-map v5 slot and checks it before anything is published.
     *
     * @param slotBytes                 Validated primary bytes of the slot.
     * @param slotName                  A slot of the MM6 profile, see `isMM6SaveSlot`, or the disposable test
     *                                  candidate.
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

    /**
     * Publishes the staged party, NPC, timer, overlay, map delta, chest, persistent event, cursor and shop owners and
     * requests the first map with a savegame load. Called once, with this attempt on the engine and before world
     * preparation. Owners published before a throw stay partial.
     *
     * @param title                     Must be MM6.
     */
    void publishOwners(GameTitle title);
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
    V5FirstMapLoadAttempt(std::unique_ptr<V5FullSessionPreflight> staged,
                          std::string_view slotName, GameTitle title);
    void notePublished(GameTitle title);
    bool chestOwnersUnchanged() const;

    std::unique_ptr<V5FullSessionPreflight> _staged;
    Phase _phase = Phase::Detached;
};

/**
 * Checks the world the engine's first-map load attempt prepared against its staged owners, arms manual saving with
 * the slot's v4 anchor, then drops the attempt. Called after world preparation and before the first input frame. A
 * refused check abandons the load instead.
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

/**
 * Sends a failed MM6 main-menu load back to the main menu before any input, world update or save, dropping its
 * first-map load attempt if it got as far as putting one on the engine.
 *
 * @return                              Whether the running title is MM6. Otherwise nothing changes.
 */
bool abandonMainMenuLoad() noexcept;

namespace testing {
/**
 * @param fault                         Called by every owner publication after the party is assigned and before the
 *                                      other owners. Empty to clear it.
 */
void setPartyPublicationFaultForTest(std::function<void()> fault);
} // namespace testing

} // namespace mm6
