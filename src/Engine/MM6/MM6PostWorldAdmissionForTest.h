#pragma once

#include <functional>
#include <stdexcept>
#include <utility>

#include "Engine/GameTitle.h"

namespace mm6::testing {

class ScopedPostWorldAdmissionForTest;

enum class PostWorldAdmissionResult {
    Unarmed,
    Continue,
    AbortToMainMenu,
};

namespace detail {

inline ScopedPostWorldAdmissionForTest *&activePostWorldAdmissionScope() {
    static thread_local ScopedPostWorldAdmissionForTest *active = nullptr;
    return active;
}

} // namespace detail

// Arm only in a disposable MM6 child's test load callback, on the game thread.
// The scope stays alive until after Game::gameLoop has rebuilt the world. It
// clears a pending callback on teardown and cannot be nested or copied. An
// explicit AbortToMainMenu result is consumed by Game::gameLoop before timer
// unpause and its first input loop; no unarmed or foreign-title call can abort.
class ScopedPostWorldAdmissionForTest {
 public:
    ScopedPostWorldAdmissionForTest(GameTitle title, std::function<void()> callback,
                                    PostWorldAdmissionResult result = PostWorldAdmissionResult::Continue)
        : _callback(std::move(callback)), _result(result) {
        if (title != GameTitle::MM6 || !_callback ||
            detail::activePostWorldAdmissionScope() ||
            (result != PostWorldAdmissionResult::Continue &&
             result != PostWorldAdmissionResult::AbortToMainMenu))
            throw std::runtime_error("MM6 post-world test admission requires a sole callback");
        detail::activePostWorldAdmissionScope() = this;
    }

    ScopedPostWorldAdmissionForTest(const ScopedPostWorldAdmissionForTest &) = delete;
    ScopedPostWorldAdmissionForTest &operator=(const ScopedPostWorldAdmissionForTest &) = delete;

    ~ScopedPostWorldAdmissionForTest() {
        if (detail::activePostWorldAdmissionScope() == this)
            detail::activePostWorldAdmissionScope() = nullptr;
    }

 private:
    friend PostWorldAdmissionResult runArmedPostWorldAdmissionForTest(GameTitle title);
    std::function<void()> _callback;
    PostWorldAdmissionResult _result;
    bool _pending = true;
};

// Called only after DoPrepareWorld(true), before timer unpause/input. A thrown
// callback still propagates; use an explicit AbortToMainMenu result for a
// passing refusal test. Consume before invoking it so neither a reentrant
// call nor a caught failure can run it twice. A second run while the scope
// remains installed fails closed.
inline PostWorldAdmissionResult runArmedPostWorldAdmissionForTest(GameTitle title) {
    ScopedPostWorldAdmissionForTest *scope = detail::activePostWorldAdmissionScope();
    if (!scope)
        return PostWorldAdmissionResult::Unarmed;
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 post-world test admission has a foreign title");
    if (!scope->_pending)
        throw std::runtime_error("MM6 post-world test admission was already consumed");
    scope->_pending = false;
    auto callback = std::move(scope->_callback);
    callback();
    return scope->_result;
}

} // namespace mm6::testing
