#pragma once

#include <stdexcept>

#include "Engine/GameTitle.h"
#include "Engine/MapEnums.h"

namespace mm6::testing {

class ScopedTransitionAutosaveInterceptForTest;

namespace detail {

inline ScopedTransitionAutosaveInterceptForTest *&activeTransitionAutosaveIntercept() {
    static thread_local ScopedTransitionAutosaveInterceptForTest *active = nullptr;
    return active;
}

} // namespace detail

// One disposable MM6 transition may skip the normal cross-map autosave.
// The unarmed route, including every MM7 transition, remains unchanged.
class ScopedTransitionAutosaveInterceptForTest {
 public:
    ScopedTransitionAutosaveInterceptForTest(GameTitle title, MapId source, MapId target)
        : _source(source), _target(target) {
        if (title != GameTitle::MM6 || source == MAP_INVALID ||
            target == MAP_INVALID || source == target ||
            detail::activeTransitionAutosaveIntercept())
            throw std::runtime_error("MM6 test transition autosave needs one exact map pair");
        detail::activeTransitionAutosaveIntercept() = this;
    }

    ScopedTransitionAutosaveInterceptForTest(
        const ScopedTransitionAutosaveInterceptForTest &) = delete;
    ScopedTransitionAutosaveInterceptForTest &operator=(
        const ScopedTransitionAutosaveInterceptForTest &) = delete;

    ~ScopedTransitionAutosaveInterceptForTest() {
        if (detail::activeTransitionAutosaveIntercept() == this)
            detail::activeTransitionAutosaveIntercept() = nullptr;
    }

    bool intercepted() const { return _intercepted; }

 private:
    friend bool interceptTransitionAutosaveForTest(GameTitle, MapId, MapId);

    MapId _source;
    MapId _target;
    bool _intercepted = false;
};

// Called only at startMapTransition's cross-map autosave branch. A mismatch
// refuses before scheduling any destination, and a scope is single-use.
inline bool interceptTransitionAutosaveForTest(
    GameTitle title, MapId source, MapId target) {
    auto *scope = detail::activeTransitionAutosaveIntercept();
    if (!scope)
        return false;
    if (title != GameTitle::MM6 || source != scope->_source ||
        target != scope->_target || scope->_intercepted)
        throw std::runtime_error("MM6 test transition autosave map or order changed");
    scope->_intercepted = true;
    return true;
}

} // namespace mm6::testing
