#pragma once

#include "Engine/GameTitle.h"

namespace mm6 {
struct V5FullSessionPreflight;
}

namespace mm6::testing {

// Arm only after detached first-map v5 preflight in a disposable fresh child.
// This is an abandon policy, not rollback: published world owners may remain
// partial, but the game loop must return to Main Menu before input or saves.
class ScopedV5FirstMapLoadAbandonForTest {
 public:
    ScopedV5FirstMapLoadAbandonForTest(
        const V5FullSessionPreflight &staged, GameTitle title);
    ScopedV5FirstMapLoadAbandonForTest(const ScopedV5FirstMapLoadAbandonForTest &) = delete;
    ScopedV5FirstMapLoadAbandonForTest &operator=(const ScopedV5FirstMapLoadAbandonForTest &) = delete;
    ~ScopedV5FirstMapLoadAbandonForTest();

    bool abandoned() const { return _abandoned; }

 private:
    friend bool abandonV5FirstMapLoadFailureForTest() noexcept;
    bool _abandoned = false;
};

// Called by Game::gameLoop only while unwinding a load or world-preparation
// exception. Returns false unless the exact MM6 test transaction is armed.
bool abandonV5FirstMapLoadFailureForTest() noexcept;

} // namespace mm6::testing
