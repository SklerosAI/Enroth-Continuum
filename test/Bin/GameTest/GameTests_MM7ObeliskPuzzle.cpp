#include <algorithm>
#include <array>
#include <filesystem>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/Evt/Processor.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Interaction.h"
#include "Engine/MM7/MM7AtomicSaveFile.h"
#include "Engine/MM7/MM7SaveValidation.h"
#include "Engine/MapEnumFunctions.h"
#include "Engine/MapEnums.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/Decoration.h"
#include "Engine/Party.h"
#include "Engine/PartyEnums.h"
#include "Engine/PartyPlacement.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Resources/LOD.h"
#include "Engine/SaveLoad.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UISaveLoad.h"

namespace {

constexpr std::string_view checkpointSlot = "saves/save000.mm7";
constexpr std::string_view checkpointFileName = "save000.mm7";
constexpr int checkpointGoldBaseline = 9000;

struct ObeliskSite {
    MapId map;
    QuestBit bit;
};

constexpr std::array obeliskSites = {
    ObeliskSite{MAP_HARMONDALE, QBIT_OBELISK_IN_HARMONDALE_FOUND},
    ObeliskSite{MAP_ERATHIA, QBIT_OBELISK_IN_ERATHIA_FOUND},
    ObeliskSite{MAP_TULAREAN_FOREST, QBIT_OBELISK_IN_TULAREAN_FOREST_FOUND},
    ObeliskSite{MAP_DEYJA, QBIT_OBELISK_IN_DEYJA_FOUND},
    ObeliskSite{MAP_BRACADA_DESERT, QBIT_OBELISK_IN_BRACADA_DESERT_FOUND},
    ObeliskSite{MAP_CELESTE, QBIT_OBELISK_IN_CELESTE_FOUND},
    ObeliskSite{MAP_PIT, QBIT_OBELISK_IN_THE_PIT_FOUND},
    ObeliskSite{MAP_EVENMORN_ISLAND, QBIT_OBELISK_IN_EVENMORN_ISLAND_FOUND},
    ObeliskSite{MAP_MOUNT_NIGHON, QBIT_OBELISK_IN_MOUNT_NIGHON_FOUND},
    ObeliskSite{MAP_BARROW_DOWNS, QBIT_OBELISK_IN_BARROW_DOWNS_FOUND},
    ObeliskSite{MAP_LAND_OF_THE_GIANTS, QBIT_OBELISK_IN_LAND_OF_THE_GIANTS_FOUND},
    ObeliskSite{MAP_TATALIA, QBIT_OBELISK_IN_TATALIA_FOUND},
    ObeliskSite{MAP_AVLEE, QBIT_OBELISK_IN_AVLEE_FOUND},
    ObeliskSite{MAP_STONE_CITY, QBIT_OBELISK_IN_STONE_CITY_FOUND},
};

bool eventSetsQuestBit(const EvtInstruction &instruction, QuestBit bit) {
    return (instruction.opcode == EVENT_Set || instruction.opcode == EVENT_Add) &&
           instruction.data.variable_descr.type == VAR_QBits_QuestsDone &&
           instruction.data.variable_descr.value == std::to_underlying(bit);
}

bool eventSetsQuestBit(int eventId, QuestBit bit) {
    return engine->_localEventMap.hasEvent(eventId) &&
           std::ranges::any_of(engine->_localEventMap.function(eventId), [bit](const EvtInstruction &instruction) {
               return eventSetsQuestBit(instruction, bit);
           });
}

void examineObelisk(EngineController &game, const ObeliskSite &site) {
    SCOPED_TRACE(fmt::format("obelisk map {} bit {}", std::to_underlying(site.map), std::to_underlying(site.bit)));
    if (isMapIndoor(site.map)) {
        game.runGameRoutine([map = site.map] {
            engine->_pendingTransition = MapDestination(map, MAP_START_POINT_PARTY);
            engineFlags |= ENGINE_SKIP_NEXT_WORLD_UPDATE;
            uGameState = GAME_STATE_CHANGE_LOCATION;
            onMapLeave();
        });
        game.tick();
        game.skipLoadingScreen();
        ASSERT_EQ(engine->_currentLoadedMapId, site.map);
    } else {
        game.teleportTo(site.map, Vec3f(), 0);
    }
    auto decoration = std::ranges::find_if(pLevelDecorations, [&site](const LevelDecoration &candidate) {
        return candidate.uEventID != 0 && eventSetsQuestBit(candidate.uEventID, site.bit);
    });
    if (decoration != pLevelDecorations.end()) {
        const int decorationId = decoration - pLevelDecorations.begin();
        DecorationInteraction(decorationId, Pid::decoration(decorationId));
    } else {
        const std::vector<BLVFace> &faces = uCurrentlyLoadedLevelType == LEVEL_OUTDOOR ? pOutdoor->faces : pIndoor->faces;
        auto face = std::ranges::find_if(faces, [&site](const BLVFace &candidate) {
            return candidate.Clickable() && candidate.eventId != 0 && eventSetsQuestBit(candidate.eventId, site.bit);
        });
        ASSERT_NE(face, faces.end());
        const int faceId = face - faces.begin();
        runEvent(EvtContext(EVT_SOURCE_MAP, face->eventId, Pid::face(faceId)));
    }
    game.tick();
    EXPECT_TRUE(pParty->_questBits[site.bit]);
}

LevelDecoration *obeliskTreasureDecoration() {
    auto treasure = std::ranges::find(pLevelDecorations, 20, &LevelDecoration::uCog);
    EXPECT_NE(treasure, pLevelDecorations.end());
    return treasure == pLevelDecorations.end() ? nullptr : &*treasure;
}

int treasureChestId(const LevelDecoration &treasure) {
    if (!engine->_localEventMap.hasEvent(treasure.uEventID))
        return -1;
    for (const EvtInstruction &instruction : engine->_localEventMap.function(treasure.uEventID)) {
        if (instruction.opcode == EVENT_OpenChest)
            return instruction.data.chest_id;
    }
    return -1;
}

void assertDisposableHeadlessProfile() {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    const auto profileRoot = mm7::startupNativeProfileRoot();
    ASSERT_TRUE(profileRoot);
    ASSERT_EQ(*profileRoot, std::filesystem::canonical(std::filesystem::path(ufs->displayPath(""))));
    ASSERT_EQ(window->nativeHandle(), nullptr);
    ASSERT_NE(dynamic_cast<NullRenderer *>(render), nullptr);
}

void runObeliskPuzzleRoute(EngineController &game, bool useCheckpointBaseline = false) {
    game.startNewGame();
    if (useCheckpointBaseline)
        pParty->SetGold(checkpointGoldBaseline);
    const int initialGold = pParty->uNumGold;

    for (const ObeliskSite &site : obeliskSites | std::views::take(obeliskSites.size() - 1))
        examineObelisk(game, site);

    game.teleportTo(MAP_EVENMORN_ISLAND, Vec3f(), 0);
    LevelDecoration *treasure = obeliskTreasureDecoration();
    ASSERT_NE(treasure, nullptr);
    pParty->uCurrentHour = 0;
    EXPECT_FALSE(treasure->IsObeliskChestActive());
    EXPECT_TRUE(treasure->uFlags & LEVEL_DECORATION_INVISIBLE);

    examineObelisk(game, obeliskSites.back());
    game.teleportTo(MAP_EVENMORN_ISLAND, Vec3f(), 0);
    treasure = obeliskTreasureDecoration();
    ASSERT_NE(treasure, nullptr);
    pParty->uCurrentHour = 1;
    EXPECT_FALSE(treasure->IsObeliskChestActive());
    EXPECT_TRUE(treasure->uFlags & LEVEL_DECORATION_INVISIBLE);
    pParty->uCurrentHour = 0;
    ASSERT_TRUE(treasure->IsObeliskChestActive());
    EXPECT_FALSE(treasure->uFlags & LEVEL_DECORATION_INVISIBLE);

    const int chestId = treasureChestId(*treasure);
    ASSERT_GE(chestId, 0);
    ASSERT_LT(chestId, vChests.size());
    std::vector<ItemId> treasureItems;
    int treasureGold = 0;
    for (InventoryEntry entry : vChests[chestId].inventory.entries()) {
        if (entry->isGold())
            treasureGold += entry->goldAmount;
        else
            treasureItems.push_back(entry->itemId);
    }
    ASSERT_FALSE(treasureItems.empty());

    const int decorationId = treasure - pLevelDecorations.data();
    DecorationInteraction(decorationId, Pid::decoration(decorationId));
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_TRUE(pParty->_questBits[QBIT_OBELISK_TREASURE_FOUND]);
    EXPECT_EQ(pParty->uNumGold, initialGold + 100000);
    for (int attempt = 0; attempt < 20 && !std::ranges::empty(vChests[chestId].inventory.entries()); ++attempt) {
        if (pParty->pPickedItem.itemId != ITEM_NULL)
            pParty->placeHeldItemInInventoryOrDrop();
        Chest::GrabItem(true);
        game.tick();
    }
    if (pParty->pPickedItem.itemId != ITEM_NULL)
        pParty->placeHeldItemInInventoryOrDrop();
    EXPECT_EQ(pParty->uNumGold, initialGold + 100000 + treasureGold);
    for (ItemId item : treasureItems)
        EXPECT_TRUE(pParty->hasItem(item));
    EXPECT_TRUE(std::ranges::empty(vChests[chestId].inventory.entries()));
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

} // namespace

GAME_TEST(MM7SideQuests, ObeliskPuzzleRequiresAllRegionsAndMidnight) {
    runObeliskPuzzleRoute(game);
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_WriteObeliskPuzzle) {
    assertDisposableHeadlessProfile();
    ASSERT_FALSE(ufs->exists(checkpointSlot));
    runObeliskPuzzleRoute(game, true);
    ASSERT_FALSE(::testing::Test::HasFatalFailure());
    ASSERT_FALSE(::testing::Test::HasFailure());
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_EVENMORN_ISLAND);
    game.runGameRoutine([] {
        saveGame(false, false, checkpointSlot, "Obelisk puzzle checkpoint");
    });

    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = ufs->read(checkpointSlot);
    ASSERT_GT(saved.size(), 1024u);
    ASSERT_TRUE(mm7::preflightNativeSaveStructure(
        std::span<const std::uint8_t>(static_cast<const std::uint8_t *>(saved.data()), saved.size()),
        GameTitle::MM7, *pGames_LOD));
}

GAME_TEST(MM7SideQuestCheckpoint, DISABLED_ReloadObeliskPuzzle) {
    assertDisposableHeadlessProfile();
    ASSERT_TRUE(ufs->exists(checkpointSlot));
    const Blob saved = Blob::copy(ufs->read(checkpointSlot));
    ASSERT_GT(saved.size(), 1024u);

    game.pressGuiButton("MainMenu_LoadGame");
    game.tick(3);
    ASSERT_EQ(current_screen_type, SCREEN_LOADGAME);
    const auto &slots = saveLoadMenu()->slots();
    const auto found = std::ranges::find(slots, checkpointFileName, &SavegameSlot::fileName);
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
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_EVENMORN_ISLAND);
    ASSERT_EQ(engine->_lastLoadedSaveFileName, checkpointFileName);
    for (const ObeliskSite &site : obeliskSites)
        EXPECT_TRUE(pParty->_questBits[site.bit]);
    EXPECT_TRUE(pParty->_questBits[QBIT_OBELISK_TREASURE_FOUND]);
    EXPECT_GT(pParty->uNumGold, checkpointGoldBaseline + 100000);
    LevelDecoration *treasure = obeliskTreasureDecoration();
    ASSERT_NE(treasure, nullptr);
    const int chestId = treasureChestId(*treasure);
    ASSERT_GE(chestId, 0);
    ASSERT_LT(chestId, vChests.size());
    EXPECT_TRUE(std::ranges::empty(vChests[chestId].inventory.entries()));
    EXPECT_TRUE(ufs->read(checkpointSlot).str() == saved.str());
}
