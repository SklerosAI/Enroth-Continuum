#pragma once

#include <functional>
#include <stdexcept>
#include <utility>

#include "Engine/GameTitle.h"
#include "Engine/MapEnums.h"

namespace mm6::testing {

class ScopedTransitionPostWorldCaptureForTest;

namespace detail {
inline ScopedTransitionPostWorldCaptureForTest *&activeTransitionPostWorldCapture() {
    static thread_local ScopedTransitionPostWorldCaptureForTest *active = nullptr;
    return active;
}
} // namespace detail

// One exact MM6 target-map observation after DoPrepareWorld and before input.
// It captures a detached candidate only; it cannot publish or write a save.
class ScopedTransitionPostWorldCaptureForTest {
 public:
    ScopedTransitionPostWorldCaptureForTest(GameTitle title, MapId target,
                                             std::function<void()> callback)
        : _target(target), _callback(std::move(callback)) {
        if (title != GameTitle::MM6 || target == MAP_INVALID || !_callback ||
            detail::activeTransitionPostWorldCapture())
            throw std::runtime_error("MM6 transition post-world capture needs one target");
        detail::activeTransitionPostWorldCapture() = this;
    }

    ScopedTransitionPostWorldCaptureForTest(
        const ScopedTransitionPostWorldCaptureForTest &) = delete;
    ScopedTransitionPostWorldCaptureForTest &operator=(
        const ScopedTransitionPostWorldCaptureForTest &) = delete;

    ~ScopedTransitionPostWorldCaptureForTest() {
        if (detail::activeTransitionPostWorldCapture() == this)
            detail::activeTransitionPostWorldCapture() = nullptr;
    }

    bool captured() const { return _captured; }

 private:
    friend void runArmedTransitionPostWorldCaptureForTest(GameTitle, MapId);
    MapId _target;
    std::function<void()> _callback;
    bool _captured = false;
};

inline void runArmedTransitionPostWorldCaptureForTest(GameTitle title,
                                                      MapId loadedMap) {
    auto *scope = detail::activeTransitionPostWorldCapture();
    if (!scope)
        return;
    if (title != GameTitle::MM6 || loadedMap != scope->_target ||
        scope->_captured)
        throw std::runtime_error("MM6 transition post-world map or order changed");
    scope->_captured = true;
    auto callback = std::move(scope->_callback);
    callback();
}

} // namespace mm6::testing
