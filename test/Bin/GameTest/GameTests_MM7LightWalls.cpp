#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <numbers>
#include <span>
#include <string_view>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MapEnums.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UIRest.h"
#include "GUI/UI/UISaveLoad.h"

namespace {

constexpr std::string_view checkpointSlot = "saves/save000.mm7";
constexpr std::string_view checkpointFileName = "save000.mm7";

std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

void runLightRoute(EngineController &game, TestController &test, CommonTapeRecorder &tapes) {
    // Light choice, city entry, the Walls quest, and its no-kill hand-in all run through their own interactions.
    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);
    auto lightTape = tapes.questBit(QBIT_LIGHT_PATH);
    auto activeTape = tapes.questBit(QBIT_WALLS_OF_MIST_ACTIVE);
    auto passedTape = tapes.questBit(QBIT_WALLS_OF_MIST_PASSED);
    auto guildTape = tapes.questBit(QBIT_LIGHT_GUILD_JOINED);
    auto mapTape = tapes.map();
    pParty->_questBits.set(QBIT_ARBITER_MESSENGER_MET);
    pParty->_questBits.set(QBIT_ARBITER_CHOICE_ACTIVE);
    pNPCStats->pNPCData[77].house = HOUSE_TAVERN_BRACADA_DESERT;
    test.startTaping();

    game.teleportTo(MAP_BRACADA_DESERT, Vec3f(-13180, 18432, 800), 0);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    game.pressGuiButton("House_Npc1"); // The Light judge.
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.teleportTo(MAP_HARMONDALE, Vec3f(20992, -6450, 672), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_LIGHT_PATH]);
    ASSERT_FALSE(pParty->_questBits[QBIT_DARK_PATH]);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();

    game.teleportTo(MAP_BRACADA_DESERT, Vec3f(-13180, 18432, 800), 0);
    auto portal = std::ranges::find_if(pOutdoor->faces, [](const BLVFace &face) { return face.eventId == 503; });
    ASSERT_NE(portal, pOutdoor->faces.end());
    ASSERT_FALSE(portal->Clickable()); // This floor trigger opens a transition when the party steps onto it.
    Vec3f portalPosition = portal->boundingBox.center();
    portalPosition.z = portal->boundingBox.z1 + 50;
    game.teleportTo(MAP_BRACADA_DESERT, portalPosition, 0);
    game.tick(5);
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CELESTE);

    auto castleDoor = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.eventId == 503 && face.Clickable();
    });
    ASSERT_NE(castleDoor, pIndoor->faces.end());
    auto [castlePosition, castleYaw] = approachFace(*castleDoor);
    game.teleportTo(MAP_CELESTE, castlePosition, castleYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_LAMBENT);
    game.teleportTo(MAP_CASTLE_LAMBENT, Vec3f(-3456, 12700, 1115), 90); // Existing Endings test door anchor.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_CASTLE_LAMBENT_THRONE_ROOM);
    ASSERT_EQ(houseNpcs.size(), 1);
    ASSERT_EQ(houseNpcs[0].npc, &pNPCStats->pNPCData[79]);
    ASSERT_FALSE(pParty->_questBits[QBIT_WALLS_OF_MIST_ACTIVE]);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_WALLS_OF_MIST_ACTIVE]);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();

    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0); // Existing town-portal arrival anchor.
    auto wallsDoor = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.eventId == 502 && face.Clickable();
    });
    ASSERT_NE(wallsDoor, pIndoor->faces.end());
    auto [wallsPosition, wallsYaw] = approachFace(*wallsDoor);
    game.teleportTo(MAP_CELESTE, wallsPosition, wallsYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_WALLS_OF_MIST);
    ASSERT_FALSE(engine->config->debug.NoActors.value());
    ASSERT_FALSE(pParty->_questBits[QBIT_WALLS_OF_MIST_PASSED]);

    constexpr std::array keys = {ITEM_WEST_PILLAR_KEY, ITEM_CENTRAL_PILLAR_KEY, ITEM_EAST_PILLAR_KEY};
    constexpr std::array pedestalFaces = {164, 4019, 4075};
    constexpr std::array positions = {Vec3f(-1122, 3392, 1), Vec3f(-482, 3264, 1), Vec3f(158, 3392, 1)};
    for (ItemId key : keys)
        pParty->pCharacters[0].inventory.add(Item(key));
    for (int i = 0; i < 3; i++) {
        game.teleportTo(MAP_WALLS_OF_MIST, positions[i], 180, -22);
        game.pointMouseAtFace(pedestalFaces[i]);
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick(2);
        ASSERT_FALSE(pParty->hasItem(keys[i]));
    }
    game.tick(20);
    auto exitDoor = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.eventId == 502 && face.Clickable();
    });
    ASSERT_NE(exitDoor, pIndoor->faces.end());
    auto [exitPosition, exitYaw] = approachFace(*exitDoor);
    game.teleportTo(MAP_WALLS_OF_MIST, exitPosition, exitYaw);
    ASSERT_FALSE(pParty->_questBits[QBIT_WALLS_OF_MIST_PASSED]);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    ASSERT_TRUE(pParty->_questBits[QBIT_WALLS_OF_MIST_PASSED]); // The exit event runs before transition acceptance.
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CELESTE);
    ASSERT_TRUE(pParty->_questBits[QBIT_WALLS_OF_MIST_ACTIVE]);

    game.teleportTo(MAP_CASTLE_LAMBENT, Vec3f(-3456, 12700, 1115), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(pNPCStats->pNPCData[79].dialogue_1_evt_id, 146);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    EXPECT_FALSE(pParty->_questBits[QBIT_WALLS_OF_MIST_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_WALLS_OF_MIST_PASSED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_LIGHT_GUILD_JOINED]);
    EXPECT_EQ(lightTape, tape(false, true));
    EXPECT_EQ(activeTape, tape(false, true, false));
    EXPECT_EQ(passedTape, tape(false, true));
    EXPECT_EQ(guildTape, tape(false, true));
    EXPECT_CONTAINS(mapTape, MAP_CELESTE);
    EXPECT_CONTAINS(mapTape, MAP_WALLS_OF_MIST);
    test.stopTaping();
}

}  // namespace

GAME_TEST(MM7LightWalls, LightRoute) {
    runLightRoute(game, test, tapes);
}

// These disabled tests require GameTestMain's exact-filter disposable MM7 profile gate.
GAME_TEST(MM7LightWalls, DISABLED_WriteFreshCheckpoint) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    ASSERT_FALSE(ufs->exists(checkpointSlot));

    runLightRoute(game, test, tapes);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(::testing::Test::HasFailure());
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_LAMBENT);
    ASSERT_TRUE(pParty->_questBits[QBIT_LIGHT_PATH]);
    ASSERT_TRUE(pParty->_questBits[QBIT_WALLS_OF_MIST_PASSED]);
    ASSERT_TRUE(pParty->_questBits[QBIT_LIGHT_GUILD_JOINED]);
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; i++) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    game.runGameRoutine([] {
        saveGame(false, false, checkpointSlot, "Light Walls checkpoint");
    });

    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

GAME_TEST(MM7LightWalls, DISABLED_ReloadFreshCheckpoint) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = Blob::copy(ufs->read(checkpointSlot));
    ASSERT_GT(saved.size(), 1024u);

    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(3);
    ASSERT_EQ(current_screen_type, SCREEN_LOADGAME);
    const auto &slots = saveLoadMenu()->slots();
    const auto found = std::find_if(slots.begin(), slots.end(), [](const SavegameSlot &entry) {
        return entry.fileName == checkpointFileName;
    });
    ASSERT_NE(found, slots.end());
    const int index = static_cast<int>(found - slots.begin());
    ASSERT_LT(index, 7);
    game.pressGuiButton(fmt::format("LoadMenu_Slot{}", index));
    game.tick(2);
    ASSERT_EQ(saveLoadMenu()->selectedSlot().fileName, checkpointFileName);
    game.pressGuiButton("LoadMenu_Load");
    game.skipLoadingScreen();
    game.tick(2);

    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_LAMBENT);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
    EXPECT_TRUE(pParty->_questBits[QBIT_LIGHT_PATH]);
    EXPECT_FALSE(pParty->_questBits[QBIT_DARK_PATH]);
    EXPECT_FALSE(pParty->_questBits[QBIT_WALLS_OF_MIST_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_WALLS_OF_MIST_PASSED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_LIGHT_GUILD_JOINED]);
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}

GAME_TEST(MM7LightWalls, ThroneClosedAtDawn) {
    // A Light-path party reaches the Lambent throne door before its daytime opening hour.
    engine->config->debug.NoActors.setValue(true); // No monster can interrupt the wait.
    game.startNewGame();
    pParty->_questBits.set(QBIT_LIGHT_PATH); // The alignment route is exercised by LightRoute.
    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_REST);
    game.pressGuiButton("Rest_WaitTillDawn");
    game.tick();
    for (int i = 0; i < 200 && currentRestType == REST_WAIT; i++)
        game.tick();
    ASSERT_EQ(currentRestType, REST_NONE);
    ASSERT_EQ(pParty->GetPlayingTime().toCivilTime().hour, 5);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.teleportTo(MAP_CASTLE_LAMBENT, Vec3f(-3456, 12700, 1115), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    EXPECT_EQ(current_screen_type, SCREEN_GAME);
    EXPECT_FALSE(pParty->_questBits[QBIT_WALLS_OF_MIST_ACTIVE]);
}
