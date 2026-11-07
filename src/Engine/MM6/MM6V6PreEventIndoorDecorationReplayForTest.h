#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Decoration.h"

#include "MM6PreEventCheckpointForTest.h"

namespace mm6::testing {

class ScopedV6PreEventIndoorDecorationReplayForTest;

namespace detail {
inline ScopedV6PreEventIndoorDecorationReplayForTest *&activeV6IndoorDecorationReplayScope() {
    static thread_local ScopedV6PreEventIndoorDecorationReplayForTest *active = nullptr;
    return active;
}
} // namespace detail

// Strict test-only publication after BLV/DLV and EVRT preparation, before
// onMapLoad. The supplied full list must already have passed v6 source staging.
class ScopedV6PreEventIndoorDecorationReplayForTest {
 public:
    ScopedV6PreEventIndoorDecorationReplayForTest(
        GameTitle title, std::string mapName, std::vector<LevelDecoration> staged)
        : _mapName(std::move(mapName)), _staged(std::move(staged)) {
        if (title != GameTitle::MM6 || !_mapName.ends_with(".blv") ||
            detail::activeV6IndoorDecorationReplayScope())
            throw std::runtime_error("MM6 v6 indoor decoration replay lacks a sole owner");
        detail::activeV6IndoorDecorationReplayScope() = this;
    }

    ScopedV6PreEventIndoorDecorationReplayForTest(
        const ScopedV6PreEventIndoorDecorationReplayForTest &) = delete;
    ScopedV6PreEventIndoorDecorationReplayForTest &operator=(
        const ScopedV6PreEventIndoorDecorationReplayForTest &) = delete;

    ~ScopedV6PreEventIndoorDecorationReplayForTest() {
        if (detail::activeV6IndoorDecorationReplayScope() == this)
            detail::activeV6IndoorDecorationReplayScope() = nullptr;
    }

    bool published() const { return _published; }

 private:
    friend bool runArmedV6PreEventIndoorDecorationReplayForTest(
        PreEventCheckpointResult, const std::vector<LevelDecoration> &,
        std::vector<LevelDecoration> *, bool, std::string_view, GameTitle);

    std::string _mapName;
    std::vector<LevelDecoration> _staged;
    bool _pending = true;
    bool _published = false;
};

inline bool runArmedV6PreEventIndoorDecorationReplayForTest(
    PreEventCheckpointResult checkpoint,
    const std::vector<LevelDecoration> &rebuilt,
    std::vector<LevelDecoration> *destination,
    bool eventRuntimeRestored, std::string_view loadedMapName,
    GameTitle title) {
    auto *scope = detail::activeV6IndoorDecorationReplayScope();
    if (!scope)
        return false;
    if (title != GameTitle::MM6 || checkpoint != PreEventCheckpointResult::Continue ||
        !scope->_pending || !eventRuntimeRestored || !destination ||
        destination != &rebuilt || loadedMapName != scope->_mapName ||
        rebuilt.size() != scope->_staged.size())
        throw std::runtime_error("MM6 v6 indoor decoration replay lacks exact pre-event context");
    for (std::size_t slot = 0; slot < rebuilt.size(); ++slot) {
        const LevelDecoration &source = rebuilt[slot];
        const LevelDecoration &saved = scope->_staged[slot];
        if (source.vPosition != saved.vPosition ||
            source._yawAngle != saved._yawAngle || source.uCog != saved.uCog ||
            source.uEventID != saved.uEventID ||
            source.uTriggerRange != saved.uTriggerRange ||
            source.eventVarId != saved.eventVarId)
            throw std::runtime_error("MM6 v6 indoor decoration source slot changed");
    }
    static_assert(std::is_nothrow_swappable_v<std::vector<LevelDecoration>>);
    destination->swap(scope->_staged);
    scope->_pending = false;
    scope->_published = true;
    return true;
}

} // namespace mm6::testing
