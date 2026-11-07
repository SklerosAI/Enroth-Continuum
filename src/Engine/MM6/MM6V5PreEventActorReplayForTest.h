#pragma once

#include <cstdint>
#include <deque>
#include <span>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Actor.h"

#include "MM6PreEventCheckpointForTest.h"
#include "MM6V5ActorRuntimeFields.h"

namespace mm6::testing {

class ScopedV5PreEventActorReplayForTest;

namespace detail {

inline ScopedV5PreEventActorReplayForTest *&activeV5ActorReplayScope() {
    static thread_local ScopedV5PreEventActorReplayForTest *active = nullptr;
    return active;
}

} // namespace detail

// ARTR is authoritative only in an explicitly armed disposable v5 child.
// The v4 ACTR baseline must first be published and match byte for byte;
// the saved dynamic list then replaces it before map events.
class ScopedV5PreEventActorReplayForTest {
 public:
    ScopedV5PreEventActorReplayForTest(
        GameTitle title, ActorRuntimeSaveFields fields,
        std::vector<std::uint8_t> baselineActr);
    ScopedV5PreEventActorReplayForTest(const ScopedV5PreEventActorReplayForTest &) = delete;
    ScopedV5PreEventActorReplayForTest &operator=(const ScopedV5PreEventActorReplayForTest &) = delete;
    ~ScopedV5PreEventActorReplayForTest();

    bool published() const { return _published; }

 private:
    friend bool runArmedV5PreEventActorReplayForTest(
        PreEventCheckpointResult checkpoint,
        const std::deque<Actor> &baseline,
        std::deque<Actor> *destination,
        bool v3Published,
        std::string_view loadedMapName,
        GameTitle title);

    ActorRuntimeSaveFields _fields;
    std::vector<std::uint8_t> _baselineActr;
    bool _pending = true;
    bool _published = false;
};

bool runArmedV5PreEventActorReplayForTest(
    PreEventCheckpointResult checkpoint,
    const std::deque<Actor> &baseline,
    std::deque<Actor> *destination,
    bool v3Published,
    std::string_view loadedMapName,
    GameTitle title);

} // namespace mm6::testing
