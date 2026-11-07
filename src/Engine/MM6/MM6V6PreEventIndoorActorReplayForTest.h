#pragma once

#include <deque>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Actor.h"

#include "MM6GeneratedNpcSaveFields.h"
#include "MM6PreEventCheckpointForTest.h"
#include "MM6V6IndoorSaveFields.h"

namespace mm6::testing {

class ScopedV6PreEventIndoorActorReplayForTest;

namespace detail {
inline ScopedV6PreEventIndoorActorReplayForTest *&activeV6IndoorActorReplayScope() {
    static thread_local ScopedV6PreEventIndoorActorReplayForTest *active = nullptr;
    return active;
}
} // namespace detail

// Test-only IART owner. Native IACR actor identities and GNPC links are
// validated before replacing the complete live deque. It is armed only for a
// disposable indoor reload after a strict v6 stage.
class ScopedV6PreEventIndoorActorReplayForTest {
 public:
    ScopedV6PreEventIndoorActorReplayForTest(
        GameTitle title, ActorRuntimeSaveFields fields,
        IndoorNativeSlotAnchor source, GeneratedNpcSaveFields generated)
        : _fields(std::move(fields)), _source(std::move(source)),
          _generated(std::move(generated)) {
        if (title != GameTitle::MM6 ||
            detail::activeV6IndoorActorReplayScope())
            throw std::runtime_error("MM6 v6 IART requires a sole title-local replay scope");
        requireIndoorActorRuntimeSourceAgreement(
            _fields, _fields.mapName, _source, title);
        const std::deque<Actor> actors(_fields.actors.begin(), _fields.actors.end());
        try {
            requireGeneratedNpcActorAgreement(
                _generated, actors, _fields.mapName, title);
        } catch (const std::exception &error) {
            throw std::runtime_error("MM6 v6 IART scoped GNPC join: " +
                                     std::string(error.what()));
        }
        detail::activeV6IndoorActorReplayScope() = this;
    }

    ScopedV6PreEventIndoorActorReplayForTest(
        const ScopedV6PreEventIndoorActorReplayForTest &) = delete;
    ScopedV6PreEventIndoorActorReplayForTest &operator=(
        const ScopedV6PreEventIndoorActorReplayForTest &) = delete;

    ~ScopedV6PreEventIndoorActorReplayForTest() {
        if (detail::activeV6IndoorActorReplayScope() == this)
            detail::activeV6IndoorActorReplayScope() = nullptr;
    }

    bool published() const { return _published; }

 private:
    friend bool runArmedV6PreEventIndoorActorReplayForTest(
        PreEventCheckpointResult, const std::deque<Actor> &,
        std::deque<Actor> *, bool, std::string_view, GameTitle);

    ActorRuntimeSaveFields _fields;
    IndoorNativeSlotAnchor _source;
    GeneratedNpcSaveFields _generated;
    bool _pending = true;
    bool _published = false;
};

inline bool runArmedV6PreEventIndoorActorReplayForTest(
    PreEventCheckpointResult checkpoint,
    const std::deque<Actor> &rebuilt, std::deque<Actor> *destination,
    bool eventRuntimeRestored, std::string_view loadedMapName,
    GameTitle title) {
    auto *scope = detail::activeV6IndoorActorReplayScope();
    if (!scope)
        return false;
    if (title != GameTitle::MM6 || checkpoint != PreEventCheckpointResult::Continue ||
        !scope->_pending || !eventRuntimeRestored || !destination ||
        destination != &rebuilt || loadedMapName != scope->_fields.mapName)
        throw std::runtime_error("MM6 v6 IART lacks exact pre-event context");
    std::vector<Actor> detached(rebuilt.begin(), rebuilt.end());
    applyIndoorActorRuntimeToDetachedList(
        scope->_fields, loadedMapName, scope->_source, &detached, title);
    std::deque<Actor> desired(detached.begin(), detached.end());
    try {
        requireGeneratedNpcActorAgreement(
            scope->_generated, desired, loadedMapName, title);
    } catch (const std::exception &error) {
        throw std::runtime_error("MM6 v6 IART pre-event GNPC join: " +
                                 std::string(error.what()));
    }
    static_assert(std::is_nothrow_swappable_v<std::deque<Actor>>);
    destination->swap(desired);
    scope->_pending = false;
    scope->_published = true;
    return true;
}

} // namespace mm6::testing
