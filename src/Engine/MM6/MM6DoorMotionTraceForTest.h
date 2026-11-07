#pragma once

#include <functional>
#include <stdexcept>
#include <utility>

#include "Engine/GameTitle.h"

struct BLVDoor;

namespace mm6::testing {

class ScopedDoorMotionTraceForTest;

namespace detail {
inline ScopedDoorMotionTraceForTest *&activeDoorMotionTraceForTest() {
    static thread_local ScopedDoorMotionTraceForTest *active = nullptr;
    return active;
}
} // namespace detail

// Records source-native door actions and updates in an armed disposable MM6 test
// load. No production save or replay path arms this scope.
class ScopedDoorMotionTraceForTest {
 public:
    ScopedDoorMotionTraceForTest(GameTitle title, std::function<void(const BLVDoor &)> callback)
        : _callback(std::move(callback)) {
        if (title != GameTitle::MM6 || !_callback ||
            detail::activeDoorMotionTraceForTest())
            throw std::runtime_error("MM6 door trace requires a sole test callback");
        detail::activeDoorMotionTraceForTest() = this;
    }

    ScopedDoorMotionTraceForTest(const ScopedDoorMotionTraceForTest &) = delete;
    ScopedDoorMotionTraceForTest &operator=(const ScopedDoorMotionTraceForTest &) = delete;

    ~ScopedDoorMotionTraceForTest() {
        if (detail::activeDoorMotionTraceForTest() == this)
            detail::activeDoorMotionTraceForTest() = nullptr;
    }

    void note(GameTitle title, const BLVDoor &door) const {
        if (title == GameTitle::MM6)
            _callback(door);
    }

 private:
    std::function<void(const BLVDoor &)> _callback;
};

inline void noteDoorMotionForTest(GameTitle title, const BLVDoor &door) {
    if (auto *scope = detail::activeDoorMotionTraceForTest())
        scope->note(title, door);
}

} // namespace mm6::testing
