#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <numbers>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnums.h"
#include "Engine/Party.h"
#include "Engine/Random/Random.h"
#include "GUI/GUIWindow.h"
#include "Utility/Exception.h"

#include "MM7CampaignChain.h"

namespace {

struct RouteStep {
    MapId from;
    int leverEvent;
    int leverIndex;
    int leverValue;
    int transitionEvent;
    MapId to;
    std::string_view label;
};

constexpr std::array allBarrowMaps = {
    MAP_BARROW_DOWNS,
    MAP_BARROW_I,
    MAP_BARROW_II,
    MAP_BARROW_III,
    MAP_BARROW_IV,
    MAP_BARROW_V,
    MAP_BARROW_VI,
    MAP_BARROW_VII,
    MAP_BARROW_VIII,
    MAP_BARROW_IX,
    MAP_BARROW_X,
    MAP_BARROW_XI,
    MAP_BARROW_XII,
    MAP_BARROW_XIII,
    MAP_BARROW_XIV,
    MAP_BARROW_XV,
    MAP_ZOKARRS_TOMB,
};

void recordVisit(std::set<MapId> *visited) {
    visited->insert(engine->_currentLoadedMapId);
}

struct FaceApproach {
    int faceId;
    Vec3f position;
    int yaw;
    int pitch;
};

bool clickBarrowEventFace(EngineController &game, MapId map, int eventId, const std::function<bool()> &done) {
    std::vector<FaceApproach> approaches;
    for (const BLVFace &face : pIndoor->faces) {
        if (face.eventId != eventId || !face.Clickable())
            continue;
        size_t approachCount = approaches.size();
        const int faceId = &face - pIndoor->faces.data();
        for (float distance : {150.0f, 250.0f, 350.0f}) {
            for (auto [direction, yaw] : {std::pair(Vec3f(1, 0, 0), 180), std::pair(Vec3f(-1, 0, 0), 0),
                                          std::pair(Vec3f(0, 1, 0), 270), std::pair(Vec3f(0, -1, 0), 90)}) {
                Vec3f position = face.boundingBox.center() + distance * direction;
                int sector = pIndoor->GetSector(position);
                int floorFace = -1;
                float floor = sector ? BLV_GetFloorLevel(position, sector, &floorFace) : -30000;
                if (floorFace == -1 || floor > face.boundingBox.z2 || floor < face.boundingBox.z1 - 200)
                    continue;
                position.z = floor;
                for (int pitch : {-15, -30, -45})
                    approaches.push_back({faceId, position, yaw, pitch});
            }
        }
        if (approaches.size() != approachCount)
            break;
    }

    for (const FaceApproach &approach : approaches) {
        game.teleportTo(map, approach.position, approach.yaw, approach.pitch);
        try {
            game.pointMouseAtFace(approach.faceId);
        } catch (const Exception &) {
            continue;
        }
        game.pressAndReleaseButton(BUTTON_LEFT);
        for (int frame = 0; frame < 8 && !done(); ++frame)
            game.tick();
        if (done())
            return true;
    }
    return false;
}

void useOutdoorBarrowEntrance(EngineController &game, int eventId, MapId destination) {
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_BARROW_DOWNS);
    auto face = std::ranges::find_if(pOutdoor->faces, [eventId](const BLVFace &candidate) {
        return candidate.eventId == eventId && candidate.Clickable();
    });
    ASSERT_NE(face, pOutdoor->faces.end());

    bool opened = false;
    for (float distance : {150.0f, 250.0f, -150.0f, -250.0f}) {
        if (opened)
            break;
        Vec3f position = face->boundingBox.center() + distance * face->facePlane.normal;
        position.z = face->boundingBox.z1;
        int yaw = std::lround(std::atan2(-face->facePlane.normal.y, -face->facePlane.normal.x) *
                              180 / std::numbers::pi);
        if (distance < 0)
            yaw += 180;
        game.teleportTo(MAP_BARROW_DOWNS, position, yaw);
        game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
        game.tick();
        opened = current_screen_type == SCREEN_INPUT_BLV;
    }
    ASSERT_TRUE(opened) << "outdoor Barrow entrance event " << eventId;
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, destination);
}

void setRouteLever(EngineController &game, MapId map, int eventId, int mapVarIndex, int value) {
    ASSERT_EQ(engine->_currentLoadedMapId, map);
    ASSERT_TRUE(clickBarrowEventFace(game, map, eventId, [mapVarIndex, value] {
        return engine->_persistentVariables.mapVars[mapVarIndex] == value;
    })) << "lever event " << eventId << " on map " << std::to_underlying(map);
    ASSERT_EQ(engine->_persistentVariables.mapVars[mapVarIndex], value);
}

void useBarrowTransition(EngineController &game, MapId map, int eventId) {
    ASSERT_EQ(engine->_currentLoadedMapId, map);
    ASSERT_TRUE(clickBarrowEventFace(game, map, eventId, [map] {
        return engine->_currentLoadedMapId != map;
    })) << "transition event " << eventId << " on map " << std::to_underlying(map);
    ASSERT_NE(engine->_currentLoadedMapId, map);
}

void followRouteStep(EngineController &game, const RouteStep &step, std::set<MapId> *visited) {
    SCOPED_TRACE(step.label);
    ASSERT_EQ(engine->_currentLoadedMapId, step.from);
    if (step.leverEvent)
        setRouteLever(game, step.from, step.leverEvent, step.leverIndex, step.leverValue);
    useBarrowTransition(game, step.from, step.transitionEvent);
    ASSERT_EQ(engine->_currentLoadedMapId, step.to);
    recordVisit(visited);
}

void seedZokarrExitForBarrowVI(EngineController &game) {
    int chosenSeed = 0;
    int nextTarget = -1;
    game.runGameRoutine([&] {
        for (int seed = 1; seed <= 1000; ++seed) {
            grng->seed(seed);
            int candidate = grng->peek(6);
            if (candidate >= 3) {
                chosenSeed = seed;
                nextTarget = candidate;
                break;
            }
        }
    });
    ASSERT_NE(chosenSeed, 0);
    ASSERT_GE(nextTarget, 3);
    ASSERT_LT(nextTarget, 6);
}

} // namespace

GAME_TEST(MM7BarrowTraversal, AllBarrowsAndZokarrThroughAuthoredTransitions) {
    engine->config->debug.NoActors.setValue(true);
    game.startNewGame();

    std::set<MapId> visited;
    game.teleportTo(MAP_BARROW_DOWNS, Vec3f(), 0);
    recordVisit(&visited);

    // The Barrow IX entrance is one of the three authored entrances from the outdoor region.
    useOutdoorBarrowEntrance(game, 503, MAP_BARROW_IX);
    recordVisit(&visited);

    constexpr std::array firstHalf = {
        RouteStep{MAP_BARROW_IX, 0, 0, 0, 502, MAP_BARROW_V, "IX -> V"},
        RouteStep{MAP_BARROW_V, 0, 0, 0, 502, MAP_BARROW_II, "V -> II"},
        RouteStep{MAP_BARROW_II, 0, 0, 0, 501, MAP_BARROW_XIV, "II -> XIV"},
        RouteStep{MAP_BARROW_XIV, 0, 0, 0, 502, MAP_BARROW_VIII, "XIV -> VIII"},
        RouteStep{MAP_BARROW_VIII, 0, 0, 0, 501, MAP_BARROW_XV, "VIII -> XV"},
        RouteStep{MAP_BARROW_XV, 0, 0, 0, 501, MAP_BARROW_IV, "XV -> IV"},
        RouteStep{MAP_BARROW_IV, 0, 0, 0, 502, MAP_ZOKARRS_TOMB, "IV -> Zokarr"},
    };
    for (const RouteStep &step : firstHalf)
        followRouteStep(game, step, &visited);

    // Zokarr's only authored exit randomly returns to IV or VI. Seed the next six-way draw into the three entries
    // that select VI, so this coverage is deterministic without replacing or bypassing the authored RandomGoTo.
    seedZokarrExitForBarrowVI(game);
    useBarrowTransition(game, MAP_ZOKARRS_TOMB, 501);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_BARROW_VI);
    recordVisit(&visited);

    constexpr std::array secondHalf = {
        RouteStep{MAP_BARROW_VI, 0, 0, 0, 501, MAP_BARROW_XI, "VI -> XI"},
        RouteStep{MAP_BARROW_XI, 0, 0, 0, 501, MAP_BARROW_I, "XI -> I"},
        RouteStep{MAP_BARROW_I, 0, 0, 0, 501, MAP_BARROW_V, "I -> V"},
        RouteStep{MAP_BARROW_V, 452, 1, 2, 501, MAP_BARROW_XII, "V -> XII"},
        RouteStep{MAP_BARROW_XII, 452, 1, 2, 501, MAP_BARROW_III, "XII -> III"},
        RouteStep{MAP_BARROW_III, 0, 0, 0, 501, MAP_BARROW_X, "III -> X"},
        RouteStep{MAP_BARROW_X, 0, 0, 0, 502, MAP_BARROW_VII, "X -> VII"},
        RouteStep{MAP_BARROW_VII, 0, 0, 0, 502, MAP_BARROW_IX, "VII -> IX"},
        RouteStep{MAP_BARROW_IX, 0, 0, 0, 502, MAP_BARROW_V, "IX -> V again"},
        RouteStep{MAP_BARROW_V, 0, 0, 0, 502, MAP_BARROW_II, "V -> II again"},
        RouteStep{MAP_BARROW_II, 451, 0, 2, 502, MAP_BARROW_XIII, "II -> XIII"},
    };
    for (const RouteStep &step : secondHalf)
        followRouteStep(game, step, &visited);

    // Return to Barrow IX and take its authored outdoor exit.
    constexpr std::array returnToExit = {
        RouteStep{MAP_BARROW_XIII, 452, 1, 2, 501, MAP_BARROW_VI, "XIII -> VI"},
        RouteStep{MAP_BARROW_VI, 0, 0, 0, 501, MAP_BARROW_XI, "VI -> XI again"},
        RouteStep{MAP_BARROW_XI, 0, 0, 0, 501, MAP_BARROW_I, "XI -> I again"},
        RouteStep{MAP_BARROW_I, 0, 0, 0, 501, MAP_BARROW_V, "I -> V again"},
        RouteStep{MAP_BARROW_V, 0, 0, 0, 501, MAP_BARROW_XII, "V -> XII again"},
        RouteStep{MAP_BARROW_XII, 452, 1, 1, 501, MAP_BARROW_V, "XII -> V"},
        RouteStep{MAP_BARROW_V, 452, 1, 1, 501, MAP_BARROW_IX, "V -> IX"},
        RouteStep{MAP_BARROW_IX, 0, 0, 0, 501, MAP_BARROW_DOWNS, "IX -> outdoors"},
    };
    for (const RouteStep &step : returnToExit)
        followRouteStep(game, step, &visited);

    EXPECT_EQ(visited, std::set<MapId>(allBarrowMaps.begin(), allBarrowMaps.end()));
}
