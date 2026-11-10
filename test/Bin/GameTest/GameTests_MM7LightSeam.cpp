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
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Graphics/Indoor.h"
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
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIHouses.h"
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

bool opensHouse(int eventId, HouseId houseId) {
    if (!engine->_localEventMap.hasEvent(eventId))
        return false;
    return std::ranges::any_of(engine->_localEventMap.function(eventId), [houseId](const EvtInstruction &instruction) {
        return instruction.opcode == EVENT_SpeakInHouse && instruction.data.house_id == houseId;
    });
}

void leaveHouse(EngineController &game) {
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; i++) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

void openCelesteHouse(EngineController &game, HouseId houseId, int npcId) {
    game.teleportTo(MAP_CELESTE, Vec3f(-158, 7624, 1), 0);
    auto door = std::ranges::find_if(pIndoor->faces, [houseId](const BLVFace &face) {
        return face.Clickable() && opensHouse(face.eventId, houseId);
    });
    ASSERT_NE(door, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*door);
    game.teleportTo(MAP_CELESTE, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), houseId);
    auto npc = std::ranges::find_if(houseNpcs, [npcId](const HouseNpcDesc &entry) {
        return entry.npc == &pNPCStats->pNPCData[npcId];
    });
    ASSERT_NE(npc, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", npc - houseNpcs.begin()));
        game.tick();
    }
}

void openThroneRoom(EngineController &game, int npcId) {
    game.teleportTo(MAP_CASTLE_LAMBENT, Vec3f(-3456, 12700, 1115), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_CASTLE_LAMBENT_THRONE_ROOM);
    auto npc = std::ranges::find_if(houseNpcs, [npcId](const HouseNpcDesc &entry) {
        return entry.npc == &pNPCStats->pNPCData[npcId];
    });
    ASSERT_NE(npc, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", npc - houseNpcs.begin()));
        game.tick();
    }
}

bool selectNpcEvent(EngineController &game, int npcId, unsigned int eventId) {
    NPCData &npc = pNPCStats->pNPCData[npcId];
    std::array eventIds = {npc.dialogue_1_evt_id, npc.dialogue_2_evt_id, npc.dialogue_3_evt_id,
                           npc.dialogue_4_evt_id, npc.dialogue_5_evt_id, npc.dialogue_6_evt_id};
    auto event = std::ranges::find(eventIds, eventId);
    if (event == eventIds.end())
        return false;
    DialogueId line = static_cast<DialogueId>(std::to_underlying(DIALOGUE_SCRIPTED_LINE_1) + (event - eventIds.begin()));
    auto topics = prepareScriptedNPCDialogueTopics(&npc);
    auto option = std::ranges::find(topics, line);
    if (option == topics.end())
        return false;
    game.pressGuiButton(fmt::format("HouseNpcDialogue_Option{}", option - topics.begin()));
    game.tick();
    return true;
}

void runLightPitSeam(EngineController &game, TestController &test) {
    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    pParty->_questBits.set(QBIT_WALLS_OF_MIST_PASSED);
    pParty->_questBits.set(QBIT_WINE_CELLAR_FINISHED);
    pParty->_questBits.set(QBIT_SOUL_JARS_LIGHT_FINISHED);
    ASSERT_EQ(pNPCStats->pNPCData[80].dialogue_1_evt_id, 147);
    ASSERT_EQ(pNPCStats->pNPCData[80].dialogue_2_evt_id, 149);
    test.startTaping();

    openThroneRoom(game, 79);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_TRUE(selectNpcEvent(game, 79, 145));
    ASSERT_TRUE(pParty->_questBits[QBIT_WALLS_OF_MIST_ACTIVE]);
    leaveHouse(game);
    openThroneRoom(game, 79);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_TRUE(selectNpcEvent(game, 79, 146));
    ASSERT_TRUE(pParty->_questBits[QBIT_LIGHT_GUILD_JOINED]);
    ASSERT_EQ(pNPCStats->pNPCData[80].house, HOUSE_CELESTE_HOSTEL_1);
    ASSERT_EQ(pNPCStats->pNPCData[83].house, HOUSE_CELESTE_HOSTEL_4);
    leaveHouse(game);

    openCelesteHouse(game, HOUSE_CELESTE_HOSTEL_1, 80);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_TRUE(selectNpcEvent(game, 80, 147));
    ASSERT_TRUE(pParty->_questBits[QBIT_ALTAR_PIECES_LIGHT_ACTIVE]);
    leaveHouse(game);
    pParty->pCharacters[0].inventory.add(Item(ITEM_QUEST_ALTAR_PIECE_LIGHT));
    pParty->pCharacters[0].inventory.add(Item(ITEM_QUEST_ALTAR_PIECE_DARK));
    openCelesteHouse(game, HOUSE_CELESTE_HOSTEL_1, 80);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_TRUE(selectNpcEvent(game, 80, 148));
    ASSERT_TRUE(pParty->_questBits[QBIT_ALTAR_PIECES_LIGHT_FINISHED]);
    ASSERT_EQ(pNPCStats->pNPCData[80].dialogue_1_evt_id, 0);
    ASSERT_EQ(pNPCStats->pNPCData[80].dialogue_2_evt_id, 149);
    leaveHouse(game);

    openCelesteHouse(game, HOUSE_CELESTE_HOSTEL_4, 83);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_TRUE(selectNpcEvent(game, 83, 155));
    ASSERT_TRUE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATION_ACTIVE]);
    leaveHouse(game);
    game.teleportTo(MAP_PIT, Vec3f(), 0);
    auto pitDoor = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) {
        return face.eventId == 376 && face.Clickable();
    });
    ASSERT_NE(pitDoor, pIndoor->faces.end());
    auto [position, yaw] = approachFace(*pitDoor);
    game.teleportTo(MAP_PIT, position, yaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_SMALL_HOUSE);

    pParty->pCharacters[0].inventory.add(Item(ITEM_QUEST_CONTROL_CUBE));
    openCelesteHouse(game, HOUSE_CELESTE_HOSTEL_4, 83);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_TRUE(selectNpcEvent(game, 83, 156));
    ASSERT_TRUE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATED]);
    ASSERT_FALSE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATION_ACTIVE]);
    ASSERT_EQ(pNPCStats->pNPCData[80].house, HOUSE_CASTLE_LAMBENT_THRONE_ROOM);
    ASSERT_EQ(pNPCStats->pNPCData[80].dialogue_1_evt_id, 0);
    ASSERT_EQ(pNPCStats->pNPCData[80].dialogue_2_evt_id, 149);
    ASSERT_EQ(pNPCStats->pNPCData[80].dialogue_4_evt_id, 223);
    leaveHouse(game);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CELESTE);
}

void requestXenofex(EngineController &game) {
    openThroneRoom(game, 80);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_TRUE(selectNpcEvent(game, 80, 149));
    EXPECT_TRUE(pParty->_questBits[QBIT_XENOFEX_LIGHT_ACTIVE]);
    EXPECT_EQ(pNPCStats->pNPCData[80].dialogue_2_evt_id, 150);
}

void assertDisposableHeadlessProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

}  // namespace

GAME_TEST(MM7LightSeam, PitToXenofexInMemory) {
    auto wallsActiveTape = tapes.questBit(QBIT_WALLS_OF_MIST_ACTIVE);
    auto altarTape = tapes.questBit(QBIT_ALTAR_PIECES_LIGHT_FINISHED);
    auto tolbertiTape = tapes.questBit(QBIT_TOLBERTI_ASSASSINATED);
    auto xenofexTape = tapes.questBit(QBIT_XENOFEX_LIGHT_ACTIVE);
    auto houseTape = tapes.custom([] { return pNPCStats->pNPCData[80].house; });
    auto firstTopicTape = tapes.custom([] { return pNPCStats->pNPCData[80].dialogue_1_evt_id; });
    auto secondTopicTape = tapes.custom([] { return pNPCStats->pNPCData[80].dialogue_2_evt_id; });
    auto fourthTopicTape = tapes.custom([] { return pNPCStats->pNPCData[80].dialogue_4_evt_id; });
    runLightPitSeam(game, test);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    requestXenofex(game);
    EXPECT_EQ(wallsActiveTape, tape(false, true, false));
    EXPECT_EQ(altarTape, tape(false, true));
    EXPECT_EQ(tolbertiTape, tape(false, true));
    EXPECT_EQ(xenofexTape, tape(false, true));
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, HOUSE_CELESTE_HOSTEL_1, HOUSE_CASTLE_LAMBENT_THRONE_ROOM));
    EXPECT_EQ(firstTopicTape, tape(147u, 148u, 0u));
    EXPECT_EQ(secondTopicTape, tape(149u, 150u));
    EXPECT_EQ(fourthTopicTape, tape(0u, 223u));
}

GAME_TEST(MM7LightSeam, DISABLED_WriteFreshCheckpoint) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(ufs->exists(checkpointSlot));
    runLightPitSeam(game, test);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(::testing::Test::HasFailure());
    game.runGameRoutine([] {
        saveGame(false, false, checkpointSlot, "Light Pit checkpoint");
    });
    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

GAME_TEST(MM7LightSeam, DISABLED_ReloadFreshCheckpoint) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
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
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CELESTE);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
    EXPECT_TRUE(pParty->_questBits[QBIT_LIGHT_PATH]);
    EXPECT_TRUE(pParty->_questBits[QBIT_LIGHT_GUILD_JOINED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_ALTAR_PIECES_LIGHT_FINISHED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_WINE_CELLAR_FINISHED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_SOUL_JARS_LIGHT_FINISHED]);
    EXPECT_TRUE(pParty->_questBits[QBIT_TOLBERTI_ASSASSINATED]);
    EXPECT_FALSE(pParty->_questBits[QBIT_XENOFEX_LIGHT_ACTIVE]);
    EXPECT_EQ(pNPCStats->pNPCData[80].house, HOUSE_CASTLE_LAMBENT_THRONE_ROOM);
    EXPECT_EQ(pNPCStats->pNPCData[80].dialogue_1_evt_id, 0);
    EXPECT_EQ(pNPCStats->pNPCData[80].dialogue_2_evt_id, 149);
    EXPECT_EQ(pNPCStats->pNPCData[80].dialogue_4_evt_id, 223);
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
    requestXenofex(game);
}
