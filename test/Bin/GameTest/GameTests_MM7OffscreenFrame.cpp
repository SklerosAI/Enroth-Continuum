#include <array>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Graphics/Renderer/Renderer.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"

#include "Utility/Exception.h"
#include "Utility/String/Format.h"

#include "MM7CombatHarness.h"
#include "MM7GameplayHarness.h"
#include "OffscreenCaptureForTest.h"

namespace {

struct CaptureStart {
    MapStartPoint point;
    PartyPlacement placement;
};

std::optional<CaptureStart> firstAvailableCaptureStart(MapId map) {
    for (MapStartPoint point : std::array{MAP_START_POINT_PARTY, MAP_START_POINT_NORTH, MAP_START_POINT_SOUTH,
                                         MAP_START_POINT_EAST, MAP_START_POINT_WEST}) {
        if (std::optional<PartyPlacement> placement = MapDestination(map, point).resolvePlacement())
            return CaptureStart{point, *placement};
    }
    return std::nullopt;
}

void loadCaptureMap(EngineController &game, MapId map) {
    if (map == engine->_currentLoadedMapId)
        return;
    game.runGameRoutine([map] {
        engine->_pendingTransition = MapDestination(map, MAP_START_POINT_PARTY);
        engineFlags |= ENGINE_SKIP_NEXT_WORLD_UPDATE;
        uGameState = GAME_STATE_CHANGE_LOCATION;
        onMapLeave();
    });
    game.tick();
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, map);
}

CaptureStart placeAtCaptureStart(MapId map) {
    const std::optional<CaptureStart> start = firstAvailableCaptureStart(map);
    EXPECT_TRUE(start);
    if (!start)
        return {};
    placeParty(start->placement);
    return *start;
}

void toughenWineCellarCaptureParty() {
    for (Character &character : pParty->pCharacters) {
        character._stats[ATTRIBUTE_SPEED] = 200;
        character._stats[ATTRIBUTE_ENDURANCE] = 200;
        character.uLevel = 200;
        character._melee_dmg_bonus = 100;
        character.sResFireBase = 1000;
        character.sResAirBase = 1000;
        character.sResWaterBase = 1000;
        character.sResEarthBase = 1000;
        character.sResPhysicalBase = 1000;
        character.sResMagicBase = 1000;
        character.sResSpiritBase = 1000;
        character.sResMindBase = 1000;
        character.sResBodyBase = 1000;
        character.sResLightBase = 1000;
        character.sResDarkBase = 1000;
        character.health = 100000;
    }
}

bool placeNearActorForInputApproach(EngineController &game, MapId map, int actorId) {
    for (const auto &[offset, yaw] : std::array{
             std::pair{Vec3f(0, -250, 0), 90}, std::pair{Vec3f(-250, 0, 0), 0},
             std::pair{Vec3f(0, 250, 0), 270}, std::pair{Vec3f(250, 0, 0), 180}}) {
        game.teleportTo(map, pActors[actorId].pos + offset, yaw);
        try {
            game.pointMouseAtActor(actorId);
            return true;
        } catch (const Exception &) {
        }
    }
    return false;
}

float horizontalDistance(const Vec3f &left, const Vec3f &right) {
    return (left - right).xy().length();
}

void runWineCellarMovementCombatScene(EngineController &game) {
    toughenWineCellarCaptureParty();
    pParty->_questBits.set(QBIT_WINE_CELLAR_ACTIVE);
    game.teleportTo(MAP_WINE_CELLAR, Vec3f(601, -512, 1), 0);

    ASSERT_FALSE(pActors.empty());
    ASSERT_EQ(pActors[0].monsterInfo.id, MONSTER_VAMPIRE_C);
    ASSERT_EQ(pActors[0].uniqueNameIndex, 5);
    ASSERT_EQ(pActors[0].aiState, Standing);
    ASSERT_TRUE(placeNearActorForInputApproach(game, MAP_WINE_CELLAR, 0));

    // All setup teleporting ends here. The measured approach, targeting and attack sequence are input-only.
    const float initialDistance = horizontalDistance(pParty->pos, pActors[0].pos);
    mm7::testing::MM7MovementResult movement = mm7::testing::driveMovementInput(game, {
        .key = PlatformKey::KEY_UP,
        .heldTicks = 8,
        .minimumHorizontalDistance = 15,
    });
    const float postMovementDistance = horizontalDistance(pParty->pos, pActors[0].pos);
    mm7::testing::CombatResult combat = mm7::testing::fightActorWithAttackInput(
        game, 0, [&] {
            try {
                game.pointMouseAtActor(0);
                return true;
            } catch (const Exception &) {
                return false;
            }
        }, 400, 1, 5);
    const float finalDistance = horizontalDistance(pParty->pos, pActors[0].pos);
    const std::string diagnostics = fmt::format(
        "distance={:.1f}->{:.1f}->{:.1f}; movement=[{}]; combat=[{}]",
        initialDistance, postMovementDistance, finalDistance, movement.diagnostic(),
        mm7::testing::describeCombatResult(combat));

    ASSERT_TRUE(movement.pressPosted) << diagnostics;
    ASSERT_TRUE(movement.releasePosted) << diagnostics;
    ASSERT_TRUE(movement.releaseProcessed) << diagnostics;
    ASSERT_LT(postMovementDistance, initialDistance) << diagnostics;
    ASSERT_EQ(movement.startMap, MAP_WINE_CELLAR) << diagnostics;
    ASSERT_EQ(movement.endMap, MAP_WINE_CELLAR) << diagnostics;
    ASSERT_TRUE(combat.targetAcquired) << diagnostics;
    ASSERT_FALSE(combat.targetingFailed) << diagnostics;
    ASSERT_TRUE(combat.observedHpDecrease) << diagnostics;
    ASSERT_TRUE(combat.reachedTerminalDeath) << diagnostics;
    ASSERT_TRUE(combat.reachedDeadState) << diagnostics;
    ASSERT_EQ(combat.gameTitle, GameTitle::MM7) << diagnostics;
    ASSERT_EQ(combat.initialMap, MAP_WINE_CELLAR) << diagnostics;
    ASSERT_EQ(combat.finalMap, MAP_WINE_CELLAR) << diagnostics;
    ASSERT_EQ(combat.initialScreen, SCREEN_GAME) << diagnostics;
    ASSERT_EQ(combat.finalScreen, SCREEN_GAME) << diagnostics;
    fmt::print("offscreen wine cellar composition: {}\n", diagnostics);
}

} // namespace

// A fresh-process, explicit macOS diagnostic. --offscreen-gl installs the
// hidden SDL OpenGL input/focus guard before GameStarter configures its window.
GAME_TEST(MM7OffscreenFrame, DISABLED_FirstOutdoorCapture) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    ASSERT_NE(window->nativeHandle(), nullptr);
    ASSERT_FALSE(window->isVisible());
    ASSERT_FALSE(window->grabsMouse());
    ASSERT_EQ(dynamic_cast<NullRenderer *>(render), nullptr);

    const char *capturePath = std::getenv("MM7_OFFSCREEN_CAPTURE_PATH");
    ASSERT_NE(capturePath, nullptr);
    ASSERT_TRUE(isOffscreenCapturePath(capturePath));

    game.startNewGame();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_EMERALD_ISLAND);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    const char *scene = std::getenv("MM7_OFFSCREEN_CAPTURE_SCENE");
    if (scene && std::string_view(scene) == "breeding-zone") {
        game.teleportTo(MAP_BREEDING_ZONE, Vec3f(), 0);
        ASSERT_EQ(engine->_currentLoadedMapId, MAP_BREEDING_ZONE);
        ASSERT_EQ(current_screen_type, SCREEN_GAME);
    } else if (scene && std::string_view(scene) == "wine-cellar-combat") {
        runWineCellarMovementCombatScene(game);
        ASSERT_FALSE(::testing::Test::HasFatalFailure());
    }
    ASSERT_FALSE(window->isVisible());
    ASSERT_FALSE(window->grabsMouse());

    const RgbaImage frame = drawOffscreenGameFrame(game);

    ASSERT_FALSE(window->isVisible());
    ASSERT_FALSE(window->grabsMouse());
    ASSERT_EQ(frame.size(), Sizei(640, 480));
    const Color first = frame[0][0];
    size_t changed = 0;
    size_t nonblackWorld = 0;
    size_t nonblackHud = 0;
    for (int y = 0; y < frame.height(); ++y) {
        for (int x = 0; x < frame.width(); ++x) {
            const Color pixel = frame[y][x];
            changed += pixel != first;
            if (pixel.r || pixel.g || pixel.b) {
                if (x < 460 && y < 350)
                    ++nonblackWorld;
                if (y >= 350)
                    ++nonblackHud;
            }
        }
    }
    EXPECT_GT(changed, 10000u);
    EXPECT_GT(nonblackWorld, 10000u);
    EXPECT_GT(nonblackHud, 10000u);

    writeOffscreenCapture(capturePath, frame);
}

GAME_TEST(MM7OffscreenFrame, DISABLED_MapCapture) {
    // Progress capture of any MM7 map from its first available start point, for enroth-capture.
    // MM7_OFFSCREEN_CAPTURE_MAP is a map file name or an index into the map table that wraps around,
    // MM7_OFFSCREEN_CAPTURE_PATH the PNG to write.
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    ASSERT_NE(window->nativeHandle(), nullptr);
    ASSERT_FALSE(window->isVisible());
    ASSERT_EQ(dynamic_cast<NullRenderer *>(render), nullptr);

    const char *capturePath = std::getenv("MM7_OFFSCREEN_CAPTURE_PATH");
    const char *mapValue = std::getenv("MM7_OFFSCREEN_CAPTURE_MAP");
    ASSERT_NE(capturePath, nullptr);
    ASSERT_NE(mapValue, nullptr);
    ASSERT_TRUE(isOffscreenCapturePath(capturePath));

    std::vector<MapId> maps;
    for (MapId id : pMapTable->pInfos.indices())
        if (!pMapTable->pInfos[id].fileName.empty())
            maps.push_back(id);
    const MapId map = offscreenCaptureMap(mapValue, maps);
    ASSERT_NE(map, MAP_INVALID) << mapValue;
    fmt::print("offscreen capture map: {}\n", pMapTable->pInfos[map].fileName);

    game.startNewGame();
    loadCaptureMap(game, map);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    placeAtCaptureStart(map);
    ASSERT_FALSE(::testing::Test::HasFailure());
    ASSERT_FALSE(window->isVisible());

    const RgbaImage frame = drawOffscreenGameFrame(game);
    ASSERT_FALSE(window->isVisible());
    ASSERT_EQ(frame.size(), Sizei(640, 480));
    writeOffscreenCapture(capturePath, frame);
}

GAME_TEST(MM7OffscreenFrame, MapCaptureUsesTargetMapStart) {
    game.startNewGame();
    for (MapId map : {MAP_TATALIA, MAP_AVLEE, MAP_SHOALS}) {
        loadCaptureMap(game, map);
        ASSERT_FALSE(::testing::Test::HasFatalFailure());
        const CaptureStart start = placeAtCaptureStart(map);
        ASSERT_FALSE(::testing::Test::HasFailure());
        EXPECT_EQ(pParty->pos, start.placement.pos);
        if (map == MAP_TATALIA || map == MAP_AVLEE)
            EXPECT_NE(start.point, MAP_START_POINT_PARTY);
    }
}
