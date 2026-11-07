#pragma once

#include <exception>
#include <functional>
#include <stdexcept>
#include <utility>

#include "Engine/GameTitle.h"
#include "Engine/MapEnums.h"

namespace mm6::testing {

// A disposable in-session return probe stops after the target outdoor world
// is prepared but before Party placement, _461103 and onMapLoad. The partial
// target is abandoned to Main Menu; this is not a rollback or publisher.
class VisitedReturnProbeAbortForTest final : public std::exception {
 public:
    const char *what() const noexcept override {
        return "MM6 visited return pre-event probe aborted to Main Menu";
    }
};

class ScopedVisitedReturnPreEventProbeForTest;

namespace detail {
inline ScopedVisitedReturnPreEventProbeForTest *&activeVisitedReturnProbe() {
    static thread_local ScopedVisitedReturnPreEventProbeForTest *active = nullptr;
    return active;
}
} // namespace detail

class ScopedVisitedReturnPreEventProbeForTest {
 public:
    ScopedVisitedReturnPreEventProbeForTest(GameTitle title, MapId target,
                                           std::function<void()> callback)
        : _target(target), _callback(std::move(callback)) {
        if (title != GameTitle::MM6 || target == MAP_INVALID || !_callback ||
            detail::activeVisitedReturnProbe())
            throw std::runtime_error("MM6 visited return probe needs one target");
        detail::activeVisitedReturnProbe() = this;
    }

    ScopedVisitedReturnPreEventProbeForTest(
        const ScopedVisitedReturnPreEventProbeForTest &) = delete;
    ScopedVisitedReturnPreEventProbeForTest &operator=(
        const ScopedVisitedReturnPreEventProbeForTest &) = delete;

    ~ScopedVisitedReturnPreEventProbeForTest() {
        if (detail::activeVisitedReturnProbe() == this)
            detail::activeVisitedReturnProbe() = nullptr;
    }

    [[nodiscard]] bool reached() const { return _reached; }

 private:
    friend void runArmedVisitedReturnPreEventProbeForTest(GameTitle, MapId);
    MapId _target;
    std::function<void()> _callback;
    bool _reached = false;
};

inline void runArmedVisitedReturnPreEventProbeForTest(GameTitle title,
                                                      MapId loadedMap) {
    auto *scope = detail::activeVisitedReturnProbe();
    if (!scope)
        return;
    if (title != GameTitle::MM6 || loadedMap != scope->_target || scope->_reached)
        throw std::runtime_error("MM6 visited return probe map/order differs");
    scope->_reached = true;
    auto callback = std::move(scope->_callback);
    callback();
    throw VisitedReturnProbeAbortForTest();
}

} // namespace mm6::testing
