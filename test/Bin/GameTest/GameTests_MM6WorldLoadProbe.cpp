#include <cmath>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/MapEnumFunctions.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6ChestSeedAndOwnership.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Party.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/DecorationTable.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/mm7_data.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIGame.h"

#include "Library/Serialization/Serialization.h"

#include "Utility/Memory/Blob.h"

#include "MM6MapEntranceForTest.h"

static bool hasPartyStartDecoration() {
    const DecorationId partyStart = pDecorationTable->decorationId(toString(MAP_START_POINT_PARTY));
    for (const LevelDecoration &decoration : pLevelDecorations)
        if (decoration.uDecorationDescID == partyStart)
            return true;
    return false;
}

GAME_TEST(MM6WorldLoadProbe, DISABLED_LoadsNamedMap) {
    // Enters the MM6 map named by ENROTH_MM6_WORLD_PROBE_MAP, a map ID or a file name, from an unsaved creation party
    // through the normal world preparation path, at the first shipped entrance into it, or at its Party Start
    // decoration when no script leads there. Run one map per process. Nothing is saved and no transition is taken.
    const char *mapValue = std::getenv("ENROTH_MM6_WORLD_PROBE_MAP");
    if (!mapValue)
        GTEST_SKIP() << "Set ENROTH_MM6_WORLD_PROBE_MAP to an MM6 map ID or file name";
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    ASSERT_TRUE(window->nativeHandle() == nullptr || !window->isVisible());
    const MapId map = mm6MapByNameOrId(mapValue);
    ASSERT_NE(map, MAP_INVALID) << mapValue;
    const std::string fileName = pMapTable->pInfos[map].fileName;
    ASSERT_TRUE(isMapOutdoor(GameTitle::MM6, map) || isMapIndoor(GameTitle::MM6, map)) << mapValue;
    const std::optional<MM6ShippedEntrance> entrance = findShippedMM6Entrance(fileName);
    std::cout << "MM6 world probe " << std::to_underlying(map) << ' ' << fileName << " entering at "
              << (entrance ? entrance->source : "Party Start") << '\n';

    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    pMapDeltas.clear();
    for (const std::string &name : pGames_LOD->ls())
        if (name.ends_with(".dlv") || name.ends_with(".ddm"))
            pMapDeltas.emplace(name, Blob::copy(pGames_LOD->read(name)));
    mm6::resetChestTitleState(&mm6::chestTitleState());
    ASSERT_TRUE(mm6::restoreChestTitleState(GameTitle::MM6, &mm6::chestTitleState(), 0x12345678u, {}));

    bool finished = false;
    std::string error;
    game.runGameRoutine([&] {
        try {
            GameUI_LoadPlayerPortraitsAndVoices();
            engine->_pendingTransition = entrance ? MapDestination(map, entrance->placement) : MapDestination(map, MAP_START_POINT_PARTY);
            mm6::ScopedChestMapLoadOptIn chestOptIn;
            DoPrepareWorld(/* bLoading = */ false, /* fullscreen = */ 1);
        } catch (const std::exception &e) {
            error = e.what();
        } catch (...) {
            error = "non-standard exception";
        }
        finished = true;
    });
    for (int tick = 0; tick < 256 && !finished; ++tick)
        game.tick();
    ASSERT_TRUE(finished);
    std::cout << "MM6 world probe " << std::to_underlying(map) << ' ' << fileName << ": ";
    if (!error.empty()) {
        std::cout << "exception: " << error << '\n';
        FAIL() << error;
    }

    const bool outdoor = isMapOutdoor(GameTitle::MM6, map);
    const bool startDecoration = hasPartyStartDecoration();
    const int sector = outdoor ? -1 : pIndoor->GetSector(pParty->pos);
    std::cout << "loaded, party start decoration " << (startDecoration ? "present" : "missing") << ", party at (" << pParty->pos.x
              << ", " << pParty->pos.y << ", " << pParty->pos.z << "), sector " << sector << '\n';
    EXPECT_EQ(engine->_currentLoadedMapId, map);
    EXPECT_EQ(uCurrentlyLoadedLevelType, outdoor ? LEVEL_OUTDOOR : LEVEL_INDOOR);
    EXPECT_FALSE(engine->_pendingTransition);
    if (entrance) {
        EXPECT_EQ(pParty->pos.x, entrance->placement.pos.x);
        EXPECT_EQ(pParty->pos.y, entrance->placement.pos.y);
    } else {
        EXPECT_TRUE(startDecoration);
    }
    if (outdoor) {
        EXPECT_FALSE(pOutdoor->mm6DeltaBytes.empty());
        EXPECT_LE(std::abs(pParty->pos.x), maxPartyAxisDistance);
        EXPECT_LE(std::abs(pParty->pos.y), maxPartyAxisDistance);
    } else {
        EXPECT_TRUE(pIndoor->bLoaded);
        EXPECT_FALSE(pIndoor->mm6DeltaBytes.empty());
        EXPECT_NE(sector, 0);
    }
}
