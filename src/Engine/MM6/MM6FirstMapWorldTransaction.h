#pragma once

#include <deque>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "Engine/GameTitle.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/SpriteObject.h"

namespace mm6 {

// A bounded test-only scratch owner for the four process collections that
// OutdoorLocation::Load and loadAndPrepareODM replace. It does not stage the
// OutdoorLocation object, Party, engine, weather, camera, UI or resources.
// Thus it must not be treated as a complete world-load transaction.
class ScopedFirstMapWorldCollections {
 public:
    ScopedFirstMapWorldCollections(GameTitle title, std::deque<Actor> *actors,
                                   std::vector<SpriteObject> *sprites,
                                   std::vector<Chest> *chests,
                                   std::vector<LevelDecoration> *decorations)
        : _actors(actors), _sprites(sprites), _chests(chests), _decorations(decorations) {
        if (title != GameTitle::MM6 || !actors || !sprites || !chests || !decorations)
            throw std::runtime_error("MM6 first-map collections require title-local owners");
        static_assert(std::is_nothrow_swappable_v<std::deque<Actor>>);
        static_assert(std::is_nothrow_swappable_v<std::vector<SpriteObject>>);
        static_assert(std::is_nothrow_swappable_v<std::vector<Chest>>);
        static_assert(std::is_nothrow_swappable_v<std::vector<LevelDecoration>>);
        _priorActors.swap(*actors);
        _priorSprites.swap(*sprites);
        _priorChests.swap(*chests);
        _priorDecorations.swap(*decorations);
        _active = true;
    }

    ScopedFirstMapWorldCollections(const ScopedFirstMapWorldCollections &) = delete;
    ScopedFirstMapWorldCollections &operator=(const ScopedFirstMapWorldCollections &) = delete;

    ~ScopedFirstMapWorldCollections() { rollback(); }

    void rollback() noexcept {
        if (!_active)
            return;
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
    std::deque<Actor> _priorActors;
    std::vector<SpriteObject> _priorSprites;
    std::vector<Chest> _priorChests;
    std::vector<LevelDecoration> _priorDecorations;
    bool _active = false;
};

} // namespace mm6
