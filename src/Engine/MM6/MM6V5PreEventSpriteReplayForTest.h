#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Objects/SpriteObject.h"

#include "MM6PreEventCheckpointForTest.h"
#include "MM6V5SpriteRuntimeFields.h"

namespace mm6::testing {

class ScopedV5PreEventSpriteReplayForTest;

namespace detail {

inline ScopedV5PreEventSpriteReplayForTest *&activeV5SpriteReplayScope() {
    static thread_local ScopedV5PreEventSpriteReplayForTest *active = nullptr;
    return active;
}

} // namespace detail

// A disposable v5 child owns a fully decoded SPRT section. Unarmed production
// MM6 and every MM7 load path are inert. An armed mismatch refuses before map
// events and before the first input frame.
class ScopedV5PreEventSpriteReplayForTest {
 public:
    ScopedV5PreEventSpriteReplayForTest(GameTitle title, SpriteRuntimeSaveFields fields);
    ScopedV5PreEventSpriteReplayForTest(const ScopedV5PreEventSpriteReplayForTest &) = delete;
    ScopedV5PreEventSpriteReplayForTest &operator=(const ScopedV5PreEventSpriteReplayForTest &) = delete;
    ~ScopedV5PreEventSpriteReplayForTest();

    bool published() const { return _published; }

 private:
    friend bool runArmedV5PreEventSpriteReplayForTest(
        PreEventCheckpointResult checkpoint,
        const std::vector<SpriteObject> &rebuilt,
        std::vector<SpriteObject> *destination,
        bool actorPublished,
        std::string_view loadedMapName,
        std::span<const std::uint8_t> activeDelta,
        GameTitle title);

    SpriteRuntimeSaveFields _fields;
    bool _pending = true;
    bool _published = false;
};

bool runArmedV5PreEventSpriteReplayForTest(
    PreEventCheckpointResult checkpoint,
    const std::vector<SpriteObject> &rebuilt,
    std::vector<SpriteObject> *destination,
    bool actorPublished,
    std::string_view loadedMapName,
    std::span<const std::uint8_t> activeDelta,
    GameTitle title);

} // namespace mm6::testing
