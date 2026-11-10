#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iterator>
#include <numbers>
#include <span>
#include <string_view>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Data/AwardEnums.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/MapEnums.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
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
} // namespace

static std::pair<Vec3f, int> approachFace(const BLVFace &face) {
    Vec3f position = face.boundingBox.center() + 150 * face.facePlane.normal;
    position.z = face.boundingBox.z1;
    int yaw = std::lround(std::atan2(-face.facePlane.normal.y, -face.facePlane.normal.x) * 180 / std::numbers::pi);
    return {position, yaw};
}

static void runRescueCase(EngineController &game, TestController &test, CommonTapeRecorder &tapes, bool missing) {
    // The Stone City king rebuilds Castle Harmondale only after all seven rescued dwarves are present at hand-in.
    SCOPED_TRACE(fmt::format("missing={}", missing));
    test.prepareForNextTest();
    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);
    game.teleportTo(MAP_HARMONDALE, Vec3f(), 0);
    auto castleDoor = std::ranges::find_if(pOutdoor->faces, [](const BLVFace &face) { return face.eventId == 301; });
    ASSERT_NE(castleDoor, pOutdoor->faces.end());
    auto [castlePosition, castleYaw] = approachFace(*castleDoor);
    game.teleportTo(MAP_HARMONDALE, castlePosition, castleYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_HARMONDALE);
    for (Actor &actor : pActors)
        if (actor.group == 5)
            actor.aiState = Dead;
    game.teleportTo(MAP_CASTLE_HARMONDALE, pParty->pos, 180);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    game.pressGuiButton("Transition_Yes");
    game.skipLoadingScreen();
    ASSERT_TRUE(pParty->_questBits[QBIT_CASTLE_HARMONDALE_GOBLINS_KILLED]);
    auto tavernDoor = std::ranges::find_if(pOutdoor->faces, [](const BLVFace &face) { return face.eventId == 15; });
    ASSERT_NE(tavernDoor, pOutdoor->faces.end());
    auto [tavernPosition, tavernYaw] = approachFace(*tavernDoor);
    game.teleportTo(MAP_HARMONDALE, tavernPosition, tavernYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    auto butler = std::ranges::find_if(houseNpcs, [](const HouseNpcDesc &npc) { return npc.npc == &pNPCStats->pNPCData[58]; });
    ASSERT_NE(butler, houseNpcs.end());
    game.pressGuiButton(fmt::format("House_Npc{}", std::distance(houseNpcs.begin(), butler)));
    game.tick();
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_CASTLE_HARMONDALE_REPAIR_ACTIVE]);
    ASSERT_TRUE(pParty->pCharacters[0]._achievedAwardsBits[AWARD_HARMONDALE_CASTLE_CLEARED]);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);

    auto rescueTape = tapes.questBit(QBIT_RED_DWARF_MINES_RESCUE_ACTIVE);
    auto repairTape = tapes.questBit(QBIT_CASTLE_HARMONDALE_REPAIR_ACTIVE);
    auto oracleTape = tapes.questBit(QBIT_ORACLE_RECOVERS_ELIXIR);
    auto rebuiltTape = tapes.questBit(QBIT_HARMONDALE_REBUILT);
    auto awardTape = tapes.custom([] { return static_cast<bool>(pParty->pCharacters[0]._achievedAwardsBits[AWARD_BARROW_DOWNS_RESCUED_MINERS]); });
    auto elixirTape = tapes.custom([] { return pParty->pPickedItem.itemId == ITEM_QUEST_ELIXIR || pParty->hasItem(ITEM_QUEST_ELIXIR); });
    auto goldTape = tapes.gold();
    auto experienceTape = tapes.totalExperience();
    auto envoysTape = tapes.custom([] { return std::pair(pNPCStats->pNPCData[67].house, pNPCStats->pNPCData[68].house); });
    auto rescuedTape = tapes.custom([] {
        int mask = 0;
        for (int npc = 60; npc <= 66; npc++)
            if (pNPCStats->pNPCData[npc].Hired())
                mask |= 1 << (npc - 60);
        return mask;
    });
    auto mapTape = tapes.map();
    auto houseTape = tapes.house();
    test.startTaping();
    game.teleportTo(MAP_STONE_CITY, Vec3f(-3100, 2176, -64), 270);
    auto throneDoor = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) { return face.eventId == 416; });
    ASSERT_NE(throneDoor, pIndoor->faces.end());
    auto [thronePosition, throneYaw] = approachFace(*throneDoor);
    game.teleportTo(MAP_STONE_CITY, thronePosition, throneYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_HOUSE);
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_STONE_CITY_THRONE_ROOM);
    auto king = std::ranges::find_if(houseNpcs, [](const HouseNpcDesc &npc) { return npc.npc == &pNPCStats->pNPCData[59]; });
    ASSERT_NE(king, houseNpcs.end());
    if (houseNpcs.size() > 1) {
        game.pressGuiButton(fmt::format("House_Npc{}", std::distance(houseNpcs.begin(), king)));
        game.tick();
    }
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();
    ASSERT_TRUE(pParty->_questBits[QBIT_RED_DWARF_MINES_RESCUE_ACTIVE]);
    ASSERT_EQ(pParty->pPickedItem.itemId, ITEM_QUEST_ELIXIR);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    game.teleportTo(MAP_RED_DWARF_MINES, Vec3f(), 0);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_RED_DWARF_MINES);
    for (int eventId = 376; eventId <= 382; eventId++) {
        if (missing && eventId == 382)
            continue;
        auto statue = std::ranges::find_if(pLevelDecorations, [eventId](const LevelDecoration &decoration) {
            return decoration.uEventID == eventId;
        });
        ASSERT_NE(statue, pLevelDecorations.end());
        const int decorationId = std::distance(pLevelDecorations.begin(), statue);
        bool pointed = false;
        for (const auto &[offset, yaw] : std::array{
                 std::pair{Vec3f(300, 0, 0), 180}, std::pair{Vec3f(-300, 0, 0), 0},
                 std::pair{Vec3f(0, 300, 0), 270}, std::pair{Vec3f(0, -300, 0), 90}}) {
            game.teleportTo(MAP_RED_DWARF_MINES, statue->vPosition + offset, yaw);
            try {
                game.pointMouseAtDecoration(decorationId);
                pointed = true;
                break;
            } catch (const Exception &) {
            }
        }
        ASSERT_TRUE(pointed);
        game.pressAndReleaseButton(BUTTON_LEFT);
        game.tick();
        int npcId = eventId == 382 ? 60 : eventId - 315;
        ASSERT_TRUE(pNPCStats->pNPCData[npcId].Hired());
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(rescuedTape.back(), missing ? 126 : 127);
    game.teleportTo(MAP_STONE_CITY, Vec3f(-3100, 2176, -64), 270);
    auto returnDoor = std::ranges::find_if(pIndoor->faces, [](const BLVFace &face) { return face.eventId == 416; });
    ASSERT_NE(returnDoor, pIndoor->faces.end());
    auto [returnPosition, returnYaw] = approachFace(*returnDoor);
    game.teleportTo(MAP_STONE_CITY, returnPosition, returnYaw);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    ASSERT_EQ(window_SpeakInHouse->houseId(), HOUSE_STONE_CITY_THRONE_ROOM);
    game.pressGuiButton("HouseNpcDialogue_Option0");
    game.tick();

    EXPECT_EQ(rescueTape, missing ? tape(false, true) : tape(false, true, false));
    EXPECT_EQ(repairTape, tape(true, false));
    EXPECT_EQ(oracleTape, missing ? tape(false, true) : tape(false, true, false));
    EXPECT_EQ(rebuiltTape, missing ? tape(false) : tape(false, true));
    EXPECT_EQ(awardTape, missing ? tape(false) : tape(false, true));
    EXPECT_EQ(elixirTape, missing ? tape(false, true) : tape(false, true, false));
    EXPECT_EQ(goldTape.delta(), missing ? 0 : 5000);
    EXPECT_EQ(experienceTape.delta(), missing ? 0 : 4 * 12500);
    EXPECT_EQ(envoysTape.size(), missing ? 1 : 2);
    if (!missing)
        EXPECT_EQ(envoysTape.back(), std::pair(HOUSE_CASTLE_HARMONDALE_THRONE_ROOM, HOUSE_CASTLE_HARMONDALE_THRONE_ROOM));
    EXPECT_EQ(rescuedTape, missing ? tape(0, 2, 6, 14, 30, 62, 126) : tape(0, 2, 6, 14, 30, 62, 126, 127, 0));
    EXPECT_EQ(mapTape, tape(MAP_HARMONDALE, MAP_STONE_CITY, MAP_RED_DWARF_MINES, MAP_STONE_CITY));
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, HOUSE_STONE_CITY_THRONE_ROOM, HOUSE_INVALID, HOUSE_STONE_CITY_THRONE_ROOM));
    test.stopTaping();
}

GAME_TEST(MM7CastleRebuild, Rescue) {
    for (bool missing : {false, true}) {
        runRescueCase(game, test, tapes, missing);
        ASSERT_FALSE(::testing::Test::HasFatalFailure());
    }
}

// These disabled tests run only with GameTestMain's exact-filter disposable MM7 profile gate.
GAME_TEST(MM7CastleRebuild, DISABLED_WriteFreshCheckpoint) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
    ASSERT_FALSE(ufs->exists(checkpointSlot));

    runRescueCase(game, test, tapes, false);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(::testing::Test::HasFailure());
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_STONE_CITY);
    ASSERT_TRUE(pParty->_questBits[QBIT_HARMONDALE_REBUILT]);
    ASSERT_EQ(pParty->uNumGold, engine->config->gameplay.NewGameGold.value() + 5000);
    for (int i = 0; i < 4 && current_screen_type != SCREEN_GAME; i++) {
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    game.runGameRoutine([] {
        saveGame(false, false, checkpointSlot, "Castle rebuild checkpoint");
    });

    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

GAME_TEST(MM7CastleRebuild, DISABLED_ReloadFreshCheckpoint) {
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
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_STONE_CITY);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
    EXPECT_TRUE(pParty->_questBits[QBIT_HARMONDALE_REBUILT]);
    EXPECT_FALSE(pParty->_questBits[QBIT_RED_DWARF_MINES_RESCUE_ACTIVE]);
    EXPECT_FALSE(pParty->_questBits[QBIT_ORACLE_RECOVERS_ELIXIR]);
    EXPECT_TRUE(pParty->pCharacters[0]._achievedAwardsBits[AWARD_HARMONDALE_CASTLE_CLEARED]);
    EXPECT_TRUE(pParty->pCharacters[0]._achievedAwardsBits[AWARD_BARROW_DOWNS_RESCUED_MINERS]);
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    EXPECT_FALSE(pParty->hasItem(ITEM_QUEST_ELIXIR));
    EXPECT_EQ(pParty->uNumGold, engine->config->gameplay.NewGameGold.value() + 5000);
    for (int npc = 60; npc <= 66; npc++)
        EXPECT_FALSE(pNPCStats->pNPCData[npc].Hired());
    EXPECT_EQ(pNPCStats->pNPCData[67].house, HOUSE_CASTLE_HARMONDALE_THRONE_ROOM);
    EXPECT_EQ(pNPCStats->pNPCData[68].house, HOUSE_CASTLE_HARMONDALE_THRONE_ROOM);
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}
