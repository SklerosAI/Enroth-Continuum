#include "MM6VisitedMapCache.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace mm6 {

std::string_view visitedMapName(const VisitedMapOwner &owner) {
    return std::visit([](const auto &entry) -> std::string_view { return entry.mapName; }, owner);
}

static void requireVisitedMapWireOwner(const VisitedMapOwner &owner, GameTitle title) {
    if (const auto *outdoor = std::get_if<VisitedOutdoorRuntimeOwner>(&owner)) {
        requireVisitedOutdoorRuntimeWireOwner(*outdoor, outdoor->mapName, title);
    } else {
        const auto &indoor = std::get<VisitedIndoorRuntimeOwner>(owner);
        requireVisitedIndoorRuntimeWireOwner(indoor, indoor.mapName, title);
    }
}

void VisitedMapCache::park(VisitedMapOwner owner, GameTitle title) {
    requireVisitedMapWireOwner(owner, title);
    const std::string_view name = visitedMapName(owner);
    const auto pos = std::lower_bound(_entries.begin(), _entries.end(), name,
        [](const VisitedMapOwner &entry, std::string_view key) { return visitedMapName(entry) < key; });
    if (pos != _entries.end() && visitedMapName(*pos) == name) {
        *pos = std::move(owner);
        return;
    }
    if (_entries.size() == MAX_MAPS)
        throw std::runtime_error("MM6 visited map cache is full");
    _entries.insert(pos, std::move(owner));
}

const VisitedMapOwner *VisitedMapCache::find(std::string_view mapName) const {
    const auto pos = std::find_if(_entries.begin(), _entries.end(),
        [&](const VisitedMapOwner &entry) { return visitedMapName(entry) == mapName; });
    return pos == _entries.end() ? nullptr : &*pos;
}

std::optional<VisitedMapOwner> VisitedMapCache::take(std::string_view mapName) {
    const auto pos = std::find_if(_entries.begin(), _entries.end(),
        [&](const VisitedMapOwner &entry) { return visitedMapName(entry) == mapName; });
    if (pos == _entries.end())
        return std::nullopt;
    VisitedMapOwner result = std::move(*pos);
    _entries.erase(pos);
    return result;
}

void VisitedMapCache::requireSeparateFrom(std::string_view activeMap) const {
    if (find(activeMap))
        throw std::runtime_error("MM6 visited map cache duplicates the active map");
}

} // namespace mm6
