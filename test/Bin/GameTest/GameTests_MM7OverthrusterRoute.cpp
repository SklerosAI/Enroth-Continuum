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
#include "Engine/Graphics/Vis.h"
#include "Engine/MapEnums.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Pid.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"
#include "GUI/UI/UISaveLoad.h"
#include "Utility/Exception.h"

namespace {

constexpr std::string_view checkpointSlot = "saves/save000.mm7";
constexpr std::string_view checkpointFileName = "save000.mm7";

std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

void leaveHouse(EngineController &game) {
    for (int i = 0; i < 3 && current_screen_type != SCREEN_GAME; i++) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
}

struct LincolnInteractionResult {
    bool pickedTarget = false;
    bool effect = false;
};

template <typename Predicate>
LincolnInteractionResult interactWithLincolnEvent(EngineController &game, int eventId, Predicate done) {
    bool pickedTarget = false;
    auto safeFloor = [](Vec3f position) {
        int sector = pIndoor->GetSector(position);
        int floorFace = -1;
        return sector != 0 &&
               BLV_GetFloorLevel(position + Vec3f(0, 0, pParty->radius), sector, &floorFace) != -30000 &&
               floorFace != -1;
    };
    for (const BLVFace &face : pIndoor->faces) {
        if (face.eventId != eventId || !face.Clickable())
            continue;
        const int faceIndex = &face - pIndoor->faces.data();
        if (std::abs(face.facePlane.normal.z) > 0.5f) {
            for (const auto &[offset, yaw] : std::array{
                     std::pair{Vec3f(300, 0, 50), 180}, std::pair{Vec3f(-300, 0, 50), 0},
                     std::pair{Vec3f(0, 300, 50), 270}, std::pair{Vec3f(0, -300, 50), 90}}) {
                Vec3f position = face.boundingBox.center() + offset;
                if (!safeFloor(position))
                    continue;
                game.teleportTo(MAP_LINCOLN, position, yaw, -45);
                try {
                    game.pointMouseAtFace(faceIndex);
                    pickedTarget |= engine->PickMouseForInteraction().pid == Pid::face(faceIndex);
                    game.pressAndReleaseButton(BUTTON_LEFT);
                    game.tick();
                    if (done())
                        return {pickedTarget, true};
                } catch (const Exception &) {
                }
            }
        }
        for (float distance : {150.0f, 300.0f, -150.0f, -300.0f}) {
            for (float height : {face.boundingBox.z1, face.boundingBox.center().z}) {
                Vec3f position = face.boundingBox.center() + distance * face.facePlane.normal;
                position.z = height;
                if (!safeFloor(position))
                    continue;
                int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
                if (distance < 0)
                    yaw += 180;
                game.teleportTo(MAP_LINCOLN, position, yaw);
                pickedTarget |= engine->PickKeyboard(engine->config->gameplay.KeyboardInteractionDepth.value(),
                                                     &vis_decoration_noevent_filter, &vis_door_filter).pid ==
                                Pid::face(faceIndex);
                game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
                game.tick();
                if (done())
                    return {pickedTarget, true};
                try {
                    game.pointMouseAtFace(faceIndex);
                    pickedTarget |= engine->PickMouseForInteraction().pid == Pid::face(faceIndex);
                    game.pressAndReleaseButton(BUTTON_LEFT);
                    game.tick();
                    if (done())
                        return {pickedTarget, true};
                } catch (const Exception &) {
                }
            }
        }
    }
    return {pickedTarget, false};
}

}  // namespace

GAME_TEST(MM7OverthrusterRoute, WetsuitBorder) {
    for (bool allSuited : {false, true}) {
        SCOPED_TRACE(fmt::format("allSuited={}", allSuited));
        test.prepareForNextTest();
        game.startNewGame();
        engine->config->debug.NoDamage.setValue(true);
        for (int i = 0; i < (allSuited ? 4 : 3); i++)
            pParty->pCharacters[i].inventory.equip(ITEM_SLOT_ARMOUR, Item(ITEM_QUEST_WETSUIT));
        game.teleportTo(MAP_AVLEE, Vec3f(-22200, 0, 1), 180);
        for (int i = 0; i < 4; i++)
            EXPECT_EQ(pParty->pCharacters[i].hasUnderwaterSuitEquipped(), allSuited || i < 3);
        auto mapTape = tapes.map();
        test.startTaping();

        game.pressKey(PlatformKey::KEY_UP);
        game.tick(12);
        game.releaseKey(PlatformKey::KEY_UP);
        if (allSuited) {
            ASSERT_EQ(current_screen_type, SCREEN_CHANGE_LOCATION);
            game.pressGuiButton("Transition_Yes");
            game.skipLoadingScreen();
            ASSERT_EQ(engine->_currentLoadedMapId, MAP_SHOALS);
            EXPECT_EQ(mapTape, tape(MAP_AVLEE, MAP_SHOALS));
        } else {
            EXPECT_EQ(current_screen_type, SCREEN_GAME);
            EXPECT_EQ(engine->_currentLoadedMapId, MAP_AVLEE);
            EXPECT_EQ(mapTape, tape(MAP_AVLEE));
        }
        test.stopTaping();
    }
}

GAME_TEST(MM7OverthrusterRoute, QuestAndWetsuitChests) {
    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    pParty->_questBits.set(QBIT_XENOFEX_QUEST_FINISHED); // The L8 hand-in is covered by the advisor lane.
    pNPCStats->pNPCData[80].house = HOUSE_CASTLE_LAMBENT_THRONE_ROOM;
    pNPCStats->pNPCData[80].dialogue_2_evt_id = 169;
    auto activeTape = tapes.questBit(QBIT_OSCILLATION_OVERTHRUSTER_LIGHT_ACTIVE);
    auto wetsuitTape = tapes.questBit(QBIT_ORACLE_RECOVERS_WETSUIT);
    test.startTaping();

    game.teleportTo(MAP_CASTLE_LAMBENT, Vec3f(-3456, 12700, 1115), 90);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    game.pressGuiButton("House_Npc1");
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_OSCILLATION_OVERTHRUSTER_LIGHT_ACTIVE]);
    ASSERT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_WETSUIT]);
    leaveHouse(game);
    ASSERT_EQ(current_screen_type, SCREEN_GAME);

    for (int eventId : {182, 183}) {
        bool opened = false;
        for (const BLVFace &face : pIndoor->faces) {
            if (face.eventId != eventId || !face.Clickable())
                continue;
            auto [position, yaw] = approachFace(face);
            game.teleportTo(MAP_CASTLE_LAMBENT, position, yaw);
            game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
            game.tick();
            if (current_screen_type == SCREEN_CHEST) {
                opened = true;
                break;
            }
        }
        ASSERT_TRUE(opened);
        ASSERT_EQ(current_screen_type, SCREEN_CHEST);
        for (int i = 0; i < 8; i++) {
            game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
            game.tick();
        }
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        ASSERT_EQ(current_screen_type, SCREEN_GAME);
    }
    EXPECT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_WETSUIT]);
    int wetsuitsCollected = 0;
    for (const Character &character : pParty->pCharacters)
        for (InventoryConstEntry entry : character.inventory.entries())
            wetsuitsCollected += entry->itemId == ITEM_QUEST_WETSUIT;
    EXPECT_GE(wetsuitsCollected, 4);
    EXPECT_EQ(activeTape, tape(false, true));
    EXPECT_EQ(wetsuitTape, tape(false, true));
    test.stopTaping();
}

GAME_TEST(MM7OverthrusterRoute, ShoalsLincolnEntry) {
    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);
    for (Character &character : pParty->pCharacters)
        character.inventory.equip(ITEM_SLOT_ARMOUR, Item(ITEM_QUEST_WETSUIT));
    game.teleportTo(MAP_SHOALS, Vec3f(), 0);
    auto mapTape = tapes.map();
    test.startTaping();

    bool opened = false;
    for (const BLVFace &face : pOutdoor->faces) {
        if (face.eventId != 501)
            continue;
        if (face.Clickable()) {
            auto [position, yaw] = approachFace(face);
            game.teleportTo(MAP_SHOALS, position, yaw);
            game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
            game.tick();
        } else {
            Vec3f position = face.boundingBox.center();
            position.z = face.boundingBox.z1 + 50;
            game.teleportTo(MAP_SHOALS, position, 0);
            game.tick(5);
        }
        if (current_screen_type == SCREEN_INPUT_BLV) {
            opened = true;
            break;
        }
    }
    ASSERT_TRUE(opened);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_LINCOLN);
    EXPECT_EQ(mapTape, tape(MAP_SHOALS, MAP_LINCOLN));
    test.stopTaping();
}

static void runLincolnRetrievalAndExit(EngineController &game, TestController &test, CommonTapeRecorder &tapes) {
    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);
    pParty->_questBits.set(QBIT_LIGHT_PATH);
    pParty->_questBits.set(QBIT_OSCILLATION_OVERTHRUSTER_LIGHT_ACTIVE); // L9 request is exercised above.
    for (int i = 0; i < 3; i++)
        pParty->pCharacters[i].inventory.equip(ITEM_SLOT_ARMOUR, Item(ITEM_QUEST_WETSUIT));
    auto takenTape = tapes.questBit(QBIT_OSCILLATION_OVERTHRUSTER_TAKEN);
    auto oracleTape = tapes.questBit(QBIT_ORACLE_RECOVERS_OSCILLATION_OVERTHRUSTER);
    auto itemTape = tapes.custom([] {
        return pParty->hasItem(ITEM_QUEST_OSCILLATION_OVERTHRUSTER) ||
               pParty->pPickedItem.itemId == ITEM_QUEST_OSCILLATION_OVERTHRUSTER;
    });
    auto mapTape = tapes.map();

    game.teleportTo(MAP_LINCOLN, Vec3f(2100, -960, 219), 180); // Existing Lincoln plate-test anchor.
    test.startTaping();
    ASSERT_EQ(engine->_persistentVariables.mapVars[2], 0);
    const auto unpowered = interactWithLincolnEvent(game, 376, [] {
        return pParty->_questBits[QBIT_OSCILLATION_OVERTHRUSTER_TAKEN];
    });
    ASSERT_TRUE(unpowered.pickedTarget);
    ASSERT_FALSE(unpowered.effect);
    ASSERT_FALSE(pParty->hasItem(ITEM_QUEST_OSCILLATION_OVERTHRUSTER));
    const auto console = interactWithLincolnEvent(game, 475, [] {
        return engine->_persistentVariables.mapVars[2] == 1;
    });
    ASSERT_TRUE(console.pickedTarget);
    ASSERT_TRUE(console.effect);
    ASSERT_FALSE(pParty->_questBits[QBIT_OSCILLATION_OVERTHRUSTER_TAKEN]);
    const auto retrieval = interactWithLincolnEvent(game, 376, [] {
        return pParty->_questBits[QBIT_OSCILLATION_OVERTHRUSTER_TAKEN];
    });
    ASSERT_TRUE(retrieval.pickedTarget);
    ASSERT_TRUE(retrieval.effect);
    ASSERT_TRUE(pParty->hasItem(ITEM_QUEST_OSCILLATION_OVERTHRUSTER) ||
                pParty->pPickedItem.itemId == ITEM_QUEST_OSCILLATION_OVERTHRUSTER);
    ASSERT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_OSCILLATION_OVERTHRUSTER]);

    ASSERT_FALSE(pParty->pCharacters[3].hasUnderwaterSuitEquipped());
    const auto unsuited = interactWithLincolnEvent(game, 501, [] {
        return current_screen_type == SCREEN_INPUT_BLV;
    });
    ASSERT_TRUE(unsuited.pickedTarget);
    ASSERT_FALSE(unsuited.effect);
    EXPECT_EQ(engine->_currentLoadedMapId, MAP_LINCOLN);
    pParty->pCharacters[3].inventory.equip(ITEM_SLOT_ARMOUR, Item(ITEM_QUEST_WETSUIT));
    const auto suited = interactWithLincolnEvent(game, 501, [] {
        return current_screen_type == SCREEN_INPUT_BLV;
    });
    ASSERT_TRUE(suited.pickedTarget);
    ASSERT_TRUE(suited.effect);
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_SHOALS);
    EXPECT_EQ(mapTape, tape(MAP_LINCOLN, MAP_SHOALS));
    EXPECT_EQ(takenTape, tape(false, true));
    EXPECT_EQ(oracleTape, tape(false, true));
    EXPECT_EQ(itemTape, tape(false, true));
    test.stopTaping();
}

GAME_TEST(MM7OverthrusterRoute, LincolnRetrievalAndExit) {
    runLincolnRetrievalAndExit(game, test, tapes);
}

// These disabled tests require GameTestMain's exact-filter disposable MM7 profile gate.
GAME_TEST(MM7OverthrusterRoute, DISABLED_WriteFreshCheckpoint) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    ASSERT_FALSE(ufs->exists(checkpointSlot));

    runLincolnRetrievalAndExit(game, test, tapes);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(::testing::Test::HasFailure());
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_SHOALS);
    ASSERT_TRUE(pParty->_questBits[QBIT_LIGHT_PATH]);
    ASSERT_TRUE(pParty->_questBits[QBIT_OSCILLATION_OVERTHRUSTER_LIGHT_ACTIVE]);
    ASSERT_TRUE(pParty->_questBits[QBIT_OSCILLATION_OVERTHRUSTER_TAKEN]);
    ASSERT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_OSCILLATION_OVERTHRUSTER]);
    for (const Character &character : pParty->pCharacters)
        ASSERT_TRUE(character.hasUnderwaterSuitEquipped());
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_OSCILLATION_OVERTHRUSTER);
    ASSERT_FALSE(pParty->hasItem(ITEM_QUEST_OSCILLATION_OVERTHRUSTER));

    SaveGameHeader header;
    game.runGameRoutine([&] {
        header = saveGame(false, false, checkpointSlot, "Overthruster retrieval checkpoint");
    });
    ASSERT_FALSE(header.locationName.empty());
    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

GAME_TEST(MM7OverthrusterRoute, DISABLED_ReloadFreshCheckpoint) {
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
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_SHOALS);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
    EXPECT_TRUE(pParty->_questBits[QBIT_LIGHT_PATH]);
    EXPECT_FALSE(pParty->_questBits[QBIT_DARK_PATH]);
    EXPECT_TRUE(pParty->_questBits[QBIT_OSCILLATION_OVERTHRUSTER_LIGHT_ACTIVE]);
    EXPECT_TRUE(pParty->_questBits[QBIT_OSCILLATION_OVERTHRUSTER_TAKEN]);
    EXPECT_TRUE(pParty->_questBits[QBIT_ORACLE_RECOVERS_OSCILLATION_OVERTHRUSTER]);
    for (const Character &character : pParty->pCharacters)
        EXPECT_TRUE(character.hasUnderwaterSuitEquipped());
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_OSCILLATION_OVERTHRUSTER);
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_OSCILLATION_OVERTHRUSTER));
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}
