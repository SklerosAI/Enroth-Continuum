#include "MM6V5PreEventDecorationReplayForTest.h"

#include <cstddef>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace mm6::testing {

ScopedV5PreEventDecorationReplayForTest::ScopedV5PreEventDecorationReplayForTest(
    GameTitle title, DecorationRuntimeSaveFields fields)
    : _fields(std::move(fields)) {
    if (title != GameTitle::MM6 || detail::activeV5DecorationReplayScope() ||
        _fields.mapName != "oute3.odm" || _fields.decorations.empty())
        throw std::runtime_error("MM6 v5 decoration replay needs one staged first-map owner");
    detail::activeV5DecorationReplayScope() = this;
}

ScopedV5PreEventDecorationReplayForTest::~ScopedV5PreEventDecorationReplayForTest() {
    if (detail::activeV5DecorationReplayScope() == this)
        detail::activeV5DecorationReplayScope() = nullptr;
}

bool runArmedV5PreEventDecorationReplayForTest(
    PreEventCheckpointResult checkpoint,
    const std::vector<LevelDecoration> &rebuilt,
    std::vector<LevelDecoration> *destination,
    bool actorPublished,
    bool eventRuntimeRestored,
    std::string_view loadedMapName,
    GameTitle title) {
    auto *scope = detail::activeV5DecorationReplayScope();
    if (!scope)
        return false;
    if (title != GameTitle::MM6 || checkpoint != PreEventCheckpointResult::Continue ||
        !scope->_pending || !actorPublished || !eventRuntimeRestored ||
        !destination || destination != &rebuilt ||
        loadedMapName != scope->_fields.mapName ||
        rebuilt.size() != scope->_fields.decorations.size())
        throw std::runtime_error("MM6 v5 decoration replay lacks an exact pre-event context");
    for (std::size_t slot = 0; slot < rebuilt.size(); ++slot) {
        const LevelDecoration &native = rebuilt[slot];
        const LevelDecoration &saved = scope->_fields.decorations[slot];
        if (native.vPosition != saved.vPosition ||
            native._yawAngle != saved._yawAngle || native.uCog != saved.uCog ||
            native.uEventID != saved.uEventID ||
            native.uTriggerRange != saved.uTriggerRange)
            throw std::runtime_error("MM6 v5 decoration source slot changed before DECR");
    }
    std::vector<LevelDecoration> restored = scope->_fields.decorations;
    static_assert(std::is_nothrow_swappable_v<std::vector<LevelDecoration>>);
    destination->swap(restored);
    scope->_pending = false;
    scope->_published = true;
    return true;
}

} // namespace mm6::testing
