#include "MM6EventRuntimeRestoreForTest.h"

#include <stdexcept>
#include <type_traits>
#include <utility>

namespace mm6::testing {

ScopedEventRuntimeRestoreForTest::ScopedEventRuntimeRestoreForTest(
    GameTitle title, EventRuntimeSaveFields fields)
    : _fields(std::move(fields)) {
    if (title != GameTitle::MM6 || detail::activeEventRuntimeRestoreScope())
        throw std::runtime_error("MM6 test event runtime requires a sole title-local scope");
    detail::activeEventRuntimeRestoreScope() = this;
}

ScopedEventRuntimeRestoreForTest::~ScopedEventRuntimeRestoreForTest() {
    if (detail::activeEventRuntimeRestoreScope() == this)
        detail::activeEventRuntimeRestoreScope() = nullptr;
}

bool runArmedEventRuntimeRestoreForTest(
    GameTitle title, std::string_view mapName, int respawnCount,
    int lastRespawnDay, bool wasRespawned, PersistentVariables *destination,
    std::span<LevelDecoration> decorations) {
    ScopedEventRuntimeRestoreForTest *scope = detail::activeEventRuntimeRestoreScope();
    if (!scope)
        return false;
    if (title != GameTitle::MM6 || scope->_restored || wasRespawned ||
        !destination || respawnCount < 0 || lastRespawnDay <= 0)
        throw EventRuntimeRestoreRefusalForTest();
    try {
        const EventRuntimeMapEntry &entry = requireEventRuntimeMapEntry(
            scope->_fields, mapName, respawnCount, lastRespawnDay,
            decorations, title);
        // The entry is completely validated before the fixed-size, noexcept
        // owner assignment. The native DDM/DLV persistent bytes are untouched.
        static_assert(std::is_nothrow_copy_assignable_v<PersistentVariables>);
        PersistentVariables updated = *destination;
        updated.mapVars = entry.mapVars;
        updated.decorVars = entry.decorVars;
        *destination = updated;
        scope->_restored = true;
        return true;
    } catch (const std::exception &) {
        throw EventRuntimeRestoreRefusalForTest();
    }
}

} // namespace mm6::testing
