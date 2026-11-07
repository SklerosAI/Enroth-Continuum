#pragma once

#include <exception>
#include <functional>
#include <stdexcept>
#include <utility>

#include "Engine/GameTitle.h"

namespace mm6::testing {

class ScopedPreEventCheckpointForTest;

enum class PreEventCheckpointResult {
    Unarmed,
    Continue,
    AbortToMainMenu,
};

// This exception is emitted only by the armed MM6 fresh-load checkpoint.
// Game::gameLoop catches it before unpausing the timer or polling input.
class PreEventCheckpointAbortForTest final : public std::exception {
 public:
    const char *what() const noexcept override {
        return "MM6 pre-event test checkpoint aborted the fresh load";
    }
};

namespace detail {

inline ScopedPreEventCheckpointForTest *&activePreEventCheckpointScope() {
    static thread_local ScopedPreEventCheckpointForTest *active = nullptr;
    return active;
}

} // namespace detail

// Arm in a disposable MM6 child's load callback on the game thread. The
// callback observes the rebuilt map before generated NPCs and OnMapReload.
// It does not admit or publish any save section. The scope must outlive the
// fresh-load gameLoop and is one-shot, non-nestable and title-locked.
class ScopedPreEventCheckpointForTest {
 public:
    ScopedPreEventCheckpointForTest(GameTitle title, std::function<void()> callback,
                                    PreEventCheckpointResult result = PreEventCheckpointResult::Continue)
        : _callback(std::move(callback)), _result(result) {
        if (title != GameTitle::MM6 || !_callback ||
            detail::activePreEventCheckpointScope() ||
            (result != PreEventCheckpointResult::Continue &&
             result != PreEventCheckpointResult::AbortToMainMenu))
            throw std::runtime_error("MM6 pre-event test checkpoint requires a sole callback");
        detail::activePreEventCheckpointScope() = this;
    }

    ScopedPreEventCheckpointForTest(const ScopedPreEventCheckpointForTest &) = delete;
    ScopedPreEventCheckpointForTest &operator=(const ScopedPreEventCheckpointForTest &) = delete;

    ~ScopedPreEventCheckpointForTest() {
        if (detail::activePreEventCheckpointScope() == this)
            detail::activePreEventCheckpointScope() = nullptr;
    }

    bool reachedMapLoad() const {
        return _reachedMapLoad;
    }

    bool reachedInput() const {
        return _reachedInput;
    }

 private:
    friend PreEventCheckpointResult runArmedPreEventCheckpointForTest(GameTitle title);
    friend void notePreEventMapLoadForTest(GameTitle title);
    friend void notePreEventInputForTest(GameTitle title);
    std::function<void()> _callback;
    PreEventCheckpointResult _result;
    bool _pending = true;
    bool _reachedMapLoad = false;
    bool _reachedInput = false;
};

inline PreEventCheckpointResult runArmedPreEventCheckpointForTest(GameTitle title) {
    ScopedPreEventCheckpointForTest *scope = detail::activePreEventCheckpointScope();
    if (!scope)
        return PreEventCheckpointResult::Unarmed;
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 pre-event test checkpoint has a foreign title");
    if (!scope->_pending)
        throw std::runtime_error("MM6 pre-event test checkpoint was already consumed");
    scope->_pending = false;
    auto callback = std::move(scope->_callback);
    callback();
    return scope->_result;
}

// Called at the actual onMapLoad call site; a test can prove an abort never
// crossed the event boundary without reading proprietary event content.
inline void notePreEventMapLoadForTest(GameTitle title) {
    ScopedPreEventCheckpointForTest *scope = detail::activePreEventCheckpointScope();
    if (!scope)
        return;
    if (title != GameTitle::MM6 || scope->_pending || scope->_reachedMapLoad)
        throw std::runtime_error("MM6 pre-event test map-load order differs");
    scope->_reachedMapLoad = true;
}

// Called immediately before the first gameplay message/input poll.
inline void notePreEventInputForTest(GameTitle title) {
    ScopedPreEventCheckpointForTest *scope = detail::activePreEventCheckpointScope();
    if (!scope)
        return;
    if (title != GameTitle::MM6 || scope->_pending || !scope->_reachedMapLoad)
        throw std::runtime_error("MM6 pre-event test input order differs");
    scope->_reachedInput = true;
}

} // namespace mm6::testing
