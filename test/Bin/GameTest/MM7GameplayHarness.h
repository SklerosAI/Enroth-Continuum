#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

#include "Engine/Components/Control/EngineController.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/MapEnums.h"
#include "Engine/Party.h"
#include "GUI/GUIEnums.h"
#include "GUI/GUIWindow.h"
#include "Library/Geometry/Vec.h"
#include "Library/Platform/Interface/PlatformEnums.h"
#include "Utility/Exception.h"
#include "Utility/String/Format.h"

namespace mm7::testing {

struct MM7MovementRequest {
    PlatformKey key = PlatformKey::KEY_NONE;
    int heldTicks = 0;
    float minimumHorizontalDistance = 0;
    int minimumYawChange = 0;
    bool requireSameMap = true;
    bool requireGameScreen = true;
};

struct MM7MovementResult {
    PlatformKey key = PlatformKey::KEY_NONE;
    MapId startMap = MAP_INVALID;
    MapId endMap = MAP_INVALID;
    Vec3f startPosition;
    Vec3f endPosition;
    Vec3f postReleasePosition;
    int startYaw = 0;
    int endYaw = 0;
    int postReleaseYaw = 0;
    int heldTicks = 0;
    bool pressPosted = false;
    bool releasePosted = false;
    bool releaseProcessed = false;

    [[nodiscard]] float horizontalDistance() const {
        return (endPosition - startPosition).xy().length();
    }

    [[nodiscard]] int yawChange() const {
        constexpr int fullTurn = 2048;
        int difference = std::abs(endYaw - startYaw) % fullTurn;
        return std::min(difference, fullTurn - difference);
    }

    [[nodiscard]] float postReleaseHorizontalDistance() const {
        return (postReleasePosition - endPosition).xy().length();
    }

    [[nodiscard]] int postReleaseYawChange() const {
        constexpr int fullTurn = 2048;
        int difference = std::abs(postReleaseYaw - endYaw) % fullTurn;
        return std::min(difference, fullTurn - difference);
    }

    [[nodiscard]] std::string diagnostic() const {
        return fmt::format(
            "key={} ticks={} press={} release={} release_processed={} map={}->{} "
            "position=({:.1f}, {:.1f}, {:.1f})->({:.1f}, {:.1f}, {:.1f})->({:.1f}, {:.1f}, {:.1f}) "
            "horizontal={:.1f}+{:.1f} yaw={}->{}->{} delta={}+{}",
            std::to_underlying(key), heldTicks, pressPosted, releasePosted, releaseProcessed,
            std::to_underlying(startMap), std::to_underlying(endMap),
            startPosition.x, startPosition.y, startPosition.z, endPosition.x, endPosition.y, endPosition.z,
            postReleasePosition.x, postReleasePosition.y, postReleasePosition.z,
            horizontalDistance(), postReleaseHorizontalDistance(), startYaw, endYaw, postReleaseYaw,
            yawChange(), postReleaseYawChange());
    }
};

namespace detail {

class ScopedHeldKey {
 public:
    ScopedHeldKey(EngineController &game, PlatformKey key): _game(game), _key(key) {
        _game.pressKey(_key);
    }

    ~ScopedHeldKey() {
        if (_held)
            _game.releaseKey(_key);
    }

    ScopedHeldKey(const ScopedHeldKey &) = delete;
    ScopedHeldKey &operator=(const ScopedHeldKey &) = delete;

    void release() {
        if (!_held)
            return;
        _game.releaseKey(_key);
        _held = false;
    }

 private:
    EngineController &_game;
    PlatformKey _key;
    bool _held = true;
};

}  // namespace detail

/**
 * Drives one bounded segment of real keyboard input and validates its observable result.
 *
 * Setup may teleport the party before this call. The measured segment cannot: this helper only posts one key press,
 * advances exactly `heldTicks` game frames, posts the matching release and advances one release frame. The returned
 * snapshots and counters make that boundary explicit in failure diagnostics.
 */
inline MM7MovementResult driveMovementInput(EngineController &game, const MM7MovementRequest &request) {
    if (engine->gameTitle() != GameTitle::MM7)
        throw Exception("MM7 movement harness used for a non-MM7 game");
    if (request.key == PlatformKey::KEY_NONE)
        throw Exception("MM7 movement harness needs a bound input key");
    if (request.heldTicks <= 0 || request.heldTicks > 512)
        throw Exception("MM7 movement harness tick bound {} is outside 1..512", request.heldTicks);
    if (current_screen_type != SCREEN_GAME)
        throw Exception("MM7 movement harness must start on the game screen, got {}", std::to_underlying(current_screen_type));

    MM7MovementResult result;
    result.key = request.key;
    result.startMap = engine->_currentLoadedMapId;
    result.startPosition = pParty->pos;
    result.startYaw = pParty->_viewYaw;

    detail::ScopedHeldKey heldKey(game, request.key);
    result.pressPosted = true;
    for (int i = 0; i < request.heldTicks; ++i) {
        game.tick();
        ++result.heldTicks;
    }
    result.endMap = engine->_currentLoadedMapId;
    result.endPosition = pParty->pos;
    result.endYaw = pParty->_viewYaw;
    heldKey.release();
    result.releasePosted = true;
    game.tick();
    result.releaseProcessed = true;
    result.postReleasePosition = pParty->pos;
    result.postReleaseYaw = pParty->_viewYaw;

    if (request.requireSameMap && (result.startMap != result.endMap || result.endMap != engine->_currentLoadedMapId))
        throw Exception("MM7 movement segment changed maps: {}", result.diagnostic());
    if (request.requireGameScreen && current_screen_type != SCREEN_GAME)
        throw Exception("MM7 movement segment left the game screen (screen={}): {}",
                        std::to_underlying(current_screen_type), result.diagnostic());
    if (result.horizontalDistance() < request.minimumHorizontalDistance)
        throw Exception("MM7 movement segment did not travel the required {:.1f} units: {}",
                        request.minimumHorizontalDistance, result.diagnostic());
    if (result.yawChange() < request.minimumYawChange)
        throw Exception("MM7 movement segment did not turn the required {} units: {}",
                        request.minimumYawChange, result.diagnostic());

    return result;
}

}  // namespace mm7::testing
