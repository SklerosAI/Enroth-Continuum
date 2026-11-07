#pragma once

#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Decoration.h"

#include "MM6PreEventCheckpointForTest.h"
#include "MM6V5DecorationRuntimeFields.h"

namespace mm6::testing {

class ScopedV5PreEventDecorationReplayForTest;

namespace detail {

inline ScopedV5PreEventDecorationReplayForTest *&activeV5DecorationReplayScope() {
    static thread_local ScopedV5PreEventDecorationReplayForTest *active = nullptr;
    return active;
}

} // namespace detail

// Test-only DECR owner. EVRT first restores its arrays against base bindings;
// PrepareDecorations then assigns event ordinals; this scope overlays the
// saved dynamic decoration list before trigger registration and map events.
class ScopedV5PreEventDecorationReplayForTest {
 public:
    ScopedV5PreEventDecorationReplayForTest(
        GameTitle title, DecorationRuntimeSaveFields fields);
    ScopedV5PreEventDecorationReplayForTest(
        const ScopedV5PreEventDecorationReplayForTest &) = delete;
    ScopedV5PreEventDecorationReplayForTest &operator=(
        const ScopedV5PreEventDecorationReplayForTest &) = delete;
    ~ScopedV5PreEventDecorationReplayForTest();

    bool published() const { return _published; }

 private:
    friend bool runArmedV5PreEventDecorationReplayForTest(
        PreEventCheckpointResult checkpoint,
        const std::vector<LevelDecoration> &rebuilt,
        std::vector<LevelDecoration> *destination,
        bool actorPublished,
        bool eventRuntimeRestored,
        std::string_view loadedMapName,
        GameTitle title);

    DecorationRuntimeSaveFields _fields;
    bool _pending = true;
    bool _published = false;
};

bool runArmedV5PreEventDecorationReplayForTest(
    PreEventCheckpointResult checkpoint,
    const std::vector<LevelDecoration> &rebuilt,
    std::vector<LevelDecoration> *destination,
    bool actorPublished,
    bool eventRuntimeRestored,
    std::string_view loadedMapName,
    GameTitle title);

} // namespace mm6::testing
