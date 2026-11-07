#include "MM6V5PreEventSpriteReplayForTest.h"

#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "Engine/Snapshots/MM6OutdoorDeltaSnapshot.h"

#include "MM6OutdoorWire.h"
#include "MM6V5OutdoorMapIdentity.h"

namespace mm6::testing {

ScopedV5PreEventSpriteReplayForTest::ScopedV5PreEventSpriteReplayForTest(
    GameTitle title, SpriteRuntimeSaveFields fields)
    : _fields(std::move(fields)) {
    if (title != GameTitle::MM6 || detail::activeV5SpriteReplayScope() ||
        !isMM6OutdoorMapName(_fields.mapName) || _fields.records.empty())
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
        !scope->_pending || !actorPublished || !destination || destination != &rebuilt ||
        loadedMapName != scope->_fields.mapName ||
        rebuilt.size() != scope->_fields.records.size())
        throw std::runtime_error("MM6 v5 sprite replay lacks an exact native map context");

    requireSpriteRuntimeNativeAgreement(
        scope->_fields, loadedMapName, activeDelta, title);
    const OutdoorDeltaWire wire = decodeOutdoorDelta(activeDelta);
    const auto ground = wire.part(OutdoorDeltaPart::GroundItems).subspan(4);
    if (wire.groundItemCount != rebuilt.size())
        throw std::runtime_error("MM6 v5 sprite count differs from native DDM");

    std::vector<SpriteObject> saved;
    saved.reserve(scope->_fields.records.size());
    for (std::size_t slot = 0; slot < scope->_fields.records.size(); ++slot) {
        const auto native = projectMM6GroundItemRecord(ground.subspan(slot * 100, 100));
        const SpriteObject &fresh = rebuilt[slot];
        const SpriteObject &restored = scope->_fields.records[slot].runtime;
        const auto sameAnchor = [&](const SpriteObject &sprite) {
            return std::to_underlying(sprite.spriteId) == native.spriteId &&
                sprite.uObjectDescID == native.objectDescId &&
                sprite.initialPosition == native.initialPosition.toFloat();
        };
        if (!sameAnchor(fresh) || !sameAnchor(restored))
            throw std::runtime_error("MM6 v5 sprite PID anchor differs from native DDM");
        saved.push_back(restored);
    }
    static_assert(std::is_nothrow_swappable_v<std::vector<SpriteObject>>);
    destination->swap(saved);
    scope->_pending = false;
    scope->_published = true;
    return true;
}

} // namespace mm6::testing
