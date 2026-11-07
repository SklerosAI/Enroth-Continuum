#pragma once

#include <deque>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "Core/Time/Time.h"
#include "Engine/GameTitle.h"
#include "Engine/LocationInfo.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Graphics/MapWeather.h"
#include "Engine/Graphics/Outdoor.h"

namespace mm6 {

using FirstMapDiscoveryGrid = std::array<std::array<uint8_t, 11>, 88>;

// The MM6 DDM-owned fields changed by OutdoorLocation::Load, plus the global
// actor slot cursor changed while reconstructing the same DDM. Callers pass
// pointers to the live OutdoorLocation fields; no OutdoorLocation is copied or
// constructed (its constructor registers a season listener).
struct FirstMapOutdoorDeltaOwners {
    LocationInfo *location = nullptr;
    std::vector<uint8_t> *wire = nullptr;
    Time *lastVisit = nullptr;
    MapWeather *weather = nullptr;
    FirstMapDiscoveryGrid *fullyRevealed = nullptr;
    FirstMapDiscoveryGrid *partiallyRevealed = nullptr;
    size_t *actorReuseCursor = nullptr;

    bool complete() const noexcept {
        return location && wire && lastVisit && weather && fullyRevealed &&
               partiallyRevealed && actorReuseCursor;
    }
};

inline FirstMapOutdoorDeltaOwners firstMapOutdoorDeltaOwners(OutdoorLocation &outdoor,
                                                             size_t &actorReuseCursor) noexcept {
    return {&outdoor.ddm, &outdoor.mm6DeltaBytes, &outdoor.lastVisitTime,
            &outdoor.weather, &outdoor.uFullyRevealedCellOnMap,
            &outdoor.uPartiallyRevealedCellOnMap, &actorReuseCursor};
}

// A bounded test-only scratch owner for the four process collections that
// OutdoorLocation::Load and loadAndPrepareODM replace. An optional joined
// scope also protects its MM6 DDM fields and the actor reuse cursor. It does
// not stage the rest of OutdoorLocation, Party, engine, global weather, camera,
// UI or resources.
// Thus it must not be treated as a complete world-load transaction.
class ScopedFirstMapWorldCollections {
 public:
    ScopedFirstMapWorldCollections(GameTitle title, std::deque<Actor> *actors,
                                   std::vector<SpriteObject> *sprites,
                                   std::vector<Chest> *chests,
                                   std::vector<LevelDecoration> *decorations,
                                   FirstMapOutdoorDeltaOwners *delta = nullptr)
        : _actors(actors), _sprites(sprites), _chests(chests), _decorations(decorations), _delta(delta) {
        if (title != GameTitle::MM6 || !actors || !sprites || !chests || !decorations ||
            (delta && !delta->complete()))
            throw std::runtime_error("MM6 first-map collections require title-local owners");
        static_assert(std::is_nothrow_swappable_v<std::deque<Actor>>);
        static_assert(std::is_nothrow_swappable_v<std::vector<SpriteObject>>);
        static_assert(std::is_nothrow_swappable_v<std::vector<Chest>>);
        static_assert(std::is_nothrow_swappable_v<std::vector<LevelDecoration>>);
        static_assert(std::is_nothrow_swappable_v<MapWeather>);
        static_assert(std::is_nothrow_swappable_v<std::vector<uint8_t>>);
        if (delta) {
            _priorLocation = *delta->location;
            _priorLastVisit = *delta->lastVisit;
            _priorFullyRevealed = *delta->fullyRevealed;
            _priorPartiallyRevealed = *delta->partiallyRevealed;
            _priorActorReuseCursor = *delta->actorReuseCursor;
        }
        _priorActors.swap(*actors);
        _priorSprites.swap(*sprites);
        _priorChests.swap(*chests);
        _priorDecorations.swap(*decorations);
        if (delta) {
            _priorWire.swap(*delta->wire);
            std::swap(_priorWeather, *delta->weather);
        }
        _active = true;
    }

    ScopedFirstMapWorldCollections(const ScopedFirstMapWorldCollections &) = delete;
    ScopedFirstMapWorldCollections &operator=(const ScopedFirstMapWorldCollections &) = delete;

    ~ScopedFirstMapWorldCollections() { rollback(); }

    void rollback() noexcept {
        if (!_active)
            return;
        if (_delta) {
            *_delta->location = _priorLocation;
            _priorWire.swap(*_delta->wire);
            *_delta->lastVisit = _priorLastVisit;
            std::swap(_priorWeather, *_delta->weather);
            *_delta->fullyRevealed = _priorFullyRevealed;
            *_delta->partiallyRevealed = _priorPartiallyRevealed;
            *_delta->actorReuseCursor = _priorActorReuseCursor;
        }
        _priorDecorations.swap(*_decorations);
        _priorChests.swap(*_chests);
        _priorSprites.swap(*_sprites);
        _priorActors.swap(*_actors);
        _active = false;
    }

    void commit() noexcept { _active = false; }

 private:
    std::deque<Actor> *_actors;
    std::vector<SpriteObject> *_sprites;
    std::vector<Chest> *_chests;
    std::vector<LevelDecoration> *_decorations;
    FirstMapOutdoorDeltaOwners *_delta;
    std::deque<Actor> _priorActors;
    std::vector<SpriteObject> _priorSprites;
    std::vector<Chest> _priorChests;
    std::vector<LevelDecoration> _priorDecorations;
    LocationInfo _priorLocation;
    std::vector<uint8_t> _priorWire;
    Time _priorLastVisit;
    MapWeather _priorWeather;
    FirstMapDiscoveryGrid _priorFullyRevealed = {};
    FirstMapDiscoveryGrid _priorPartiallyRevealed = {};
    size_t _priorActorReuseCursor = 0;
    bool _active = false;
};

} // namespace mm6
