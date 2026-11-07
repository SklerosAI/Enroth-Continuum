#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Objects/SpriteObject.h"

#include "MM6PreEventCheckpointForTest.h"
#include "MM6V6IndoorSaveFields.h"

namespace mm6::testing {

class ScopedV6PreEventIndoorSpriteReplayForTest;

namespace detail {
inline ScopedV6PreEventIndoorSpriteReplayForTest *&activeV6IndoorSpriteReplayScope() {
    static thread_local ScopedV6PreEventIndoorSpriteReplayForTest *active = nullptr;
    return active;
}
} // namespace detail

// The source-free ISCR slots and appended runtime tail may be regenerated
// differently by the loader. ISRT is the exact authoritative saved full list;
// occupied native ISCR prefix slots remain immutable.
class ScopedV6PreEventIndoorSpriteReplayForTest {
 public:
    ScopedV6PreEventIndoorSpriteReplayForTest(
        GameTitle title, SpriteRuntimeSaveFields fields,
        IndoorNativeSlotAnchor source)
        : _fields(std::move(fields)), _source(std::move(source)) {
        if (title != GameTitle::MM6 ||
            detail::activeV6IndoorSpriteReplayScope())
            throw std::runtime_error("MM6 v6 ISRT requires a sole title-local replay scope");
        requireIndoorSpriteRuntimeSourceAgreement(
            _fields, _fields.mapName, _source, title);
        detail::activeV6IndoorSpriteReplayScope() = this;
    }

    ScopedV6PreEventIndoorSpriteReplayForTest(
        const ScopedV6PreEventIndoorSpriteReplayForTest &) = delete;
    ScopedV6PreEventIndoorSpriteReplayForTest &operator=(
        const ScopedV6PreEventIndoorSpriteReplayForTest &) = delete;

    ~ScopedV6PreEventIndoorSpriteReplayForTest() {
        if (detail::activeV6IndoorSpriteReplayScope() == this)
            detail::activeV6IndoorSpriteReplayScope() = nullptr;
    }

    bool published() const { return _published; }

 private:
    friend bool runArmedV6PreEventIndoorSpriteReplayForTest(
        PreEventCheckpointResult, const std::vector<SpriteObject> &,
        std::vector<SpriteObject> *, bool, std::string_view, GameTitle);

    SpriteRuntimeSaveFields _fields;
    IndoorNativeSlotAnchor _source;
    bool _pending = true;
    bool _published = false;
};

inline bool runArmedV6PreEventIndoorSpriteReplayForTest(
    PreEventCheckpointResult checkpoint,
    const std::vector<SpriteObject> &rebuilt,
    std::vector<SpriteObject> *destination,
    bool eventRuntimeRestored, std::string_view loadedMapName,
    GameTitle title) {
    auto *scope = detail::activeV6IndoorSpriteReplayScope();
    if (!scope)
        return false;
    if (title != GameTitle::MM6 || checkpoint != PreEventCheckpointResult::Continue ||
        !scope->_pending || !eventRuntimeRestored || !destination ||
        destination != &rebuilt || loadedMapName != scope->_fields.mapName)
        throw std::runtime_error("MM6 v6 ISRT lacks exact pre-event context");
    applyIndoorSpriteRuntimeAfterMapReload(
        scope->_fields, loadedMapName, scope->_source, destination, title);
    scope->_pending = false;
    scope->_published = true;
    return true;
}

} // namespace mm6::testing
