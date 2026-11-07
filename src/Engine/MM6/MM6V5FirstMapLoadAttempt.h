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

// Engine-owned first-map v5 load context. This first slice owns the detached
// graph and the chest handoff only. Other replay phases still use disposable
// test hooks; no normal SaveLoad route may construct this context yet.
class V5FirstMapLoadAttempt {
 public:
    enum class Phase { Detached, Published, EventRestored, ChestApplied, Abandoned };

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
        return _phase == Phase::EventRestored || _phase == Phase::ChestApplied;
    }
    bool applyChestAtNativeMap(GameTitle title);
    void abandon() noexcept { _phase = Phase::Abandoned; }

 private:
    std::unique_ptr<V5FullSessionPreflight> _staged;
    Phase _phase = Phase::Detached;
};

} // namespace mm6
