#pragma once

#include <exception>
#include <span>
#include <string_view>

#include "Engine/GameTitle.h"
#include "Engine/Engine.h"

#include "MM6EventRuntimeSaveFields.h"

struct LevelDecoration;

namespace mm6::testing {

// An armed reload that cannot restore exact EVRT state aborts before
// PrepareDecorations reads the inherited event arrays. DoPrepareWorld catches
// only this test refusal and converts it to its existing pre-input abort.
class EventRuntimeRestoreRefusalForTest final : public std::exception {
 public:
    const char *what() const noexcept override {
        return "MM6 test event runtime could not be restored before decorations";
    }
};

class ScopedEventRuntimeRestoreForTest;

namespace detail {

inline ScopedEventRuntimeRestoreForTest *&activeEventRuntimeRestoreScope() {
    static thread_local ScopedEventRuntimeRestoreForTest *active = nullptr;
    return active;
}

} // namespace detail

// A detached, fully decoded v4 owner is copied when the test scope is armed.
// This scope is never armed by production SaveLoad or Atlas.
class ScopedEventRuntimeRestoreForTest {
 public:
    ScopedEventRuntimeRestoreForTest(GameTitle title, EventRuntimeSaveFields fields);
    ScopedEventRuntimeRestoreForTest(const ScopedEventRuntimeRestoreForTest &) = delete;
    ScopedEventRuntimeRestoreForTest &operator=(const ScopedEventRuntimeRestoreForTest &) = delete;
    ~ScopedEventRuntimeRestoreForTest();

    bool restored() const { return _restored; }

 private:
    friend bool runArmedEventRuntimeRestoreForTest(
        GameTitle title, std::string_view mapName, int respawnCount,
        int lastRespawnDay, bool wasRespawned, PersistentVariables *destination,
        std::span<LevelDecoration> decorations);

    EventRuntimeSaveFields _fields;
    bool _restored = false;
};

// Called after the MM6 DDM/DLV and respawn decision, before either outdoor or
// indoor decoration preparation. Unarmed calls are no-ops. An armed scope
// requires a nonrespawn reload and an exact map/header/binding match.
bool runArmedEventRuntimeRestoreForTest(
    GameTitle title, std::string_view mapName, int respawnCount,
    int lastRespawnDay, bool wasRespawned, PersistentVariables *destination,
    std::span<LevelDecoration> decorations);

} // namespace mm6::testing
