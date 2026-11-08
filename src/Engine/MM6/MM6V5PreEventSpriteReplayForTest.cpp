#include "MM6V5PreEventSpriteReplayForTest.h"

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "MM6V5OutdoorMapIdentity.h"

namespace mm6::testing {

ScopedV5PreEventSpriteReplayForTest::ScopedV5PreEventSpriteReplayForTest(
    GameTitle title, SpriteRuntimeSaveFields fields)
    : _fields(std::move(fields)) {
    if (title != GameTitle::MM6 || detail::activeV5SpriteReplayScope() ||
        !isMM6OutdoorMapName(_fields.mapName))
        throw std::runtime_error("MM6 v5 sprite replay needs one staged outdoor owner");
    detail::activeV5SpriteReplayScope() = this;
}

ScopedV5PreEventSpriteReplayForTest::~ScopedV5PreEventSpriteReplayForTest() {
    if (detail::activeV5SpriteReplayScope() == this)
        detail::activeV5SpriteReplayScope() = nullptr;
}

bool runArmedV5PreEventSpriteReplayForTest(
    PreEventCheckpointResult checkpoint,
    const std::vector<SpriteObject> &rebuilt,
    std::vector<SpriteObject> *destination,
    bool actorPublished,
    std::string_view loadedMapName,
    std::span<const std::uint8_t> activeDelta,
    GameTitle title) {
    ScopedV5PreEventSpriteReplayForTest *scope = detail::activeV5SpriteReplayScope();
    if (!scope)
        return false;
    if (title != GameTitle::MM6 || checkpoint != PreEventCheckpointResult::Continue ||
        !scope->_pending || !actorPublished || !destination || destination != &rebuilt)
        throw std::runtime_error("MM6 v5 sprite replay lacks an exact native map context");
    std::vector<SpriteObject> saved = stageSpriteRuntimeReplay(
        scope->_fields, rebuilt, loadedMapName, activeDelta, title);
    static_assert(std::is_nothrow_swappable_v<std::vector<SpriteObject>>);
    destination->swap(saved);
    scope->_pending = false;
    scope->_published = true;
    return true;
}

} // namespace mm6::testing
