#include <array>
#include <cstring>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Evt/EvtProgram.h"
#include "Engine/MapEnums.h"
#include "Engine/Mods/PrivateMods.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Party.h"
#include "Engine/Snapshots/EntitySnapshots.h"
#include "Engine/Tables/ChestTable.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/UI/UIChest.h"

namespace {

constexpr int firstStorageEvent = 176;
constexpr int lastStorageEvent = 195;
constexpr int boundaryChestId = 0;
constexpr ItemId storedArtifact = ITEM_ARTIFACT_PUCK;
constexpr ItemId storedOrdinaryItem = ITEM_POTION_CURE_WOUNDS;

void assertStorageEventContract() {
    std::array<int, 20> chestReferences = {};
    for (int eventId = firstStorageEvent; eventId <= lastStorageEvent; ++eventId) {
        ASSERT_TRUE(engine->_localEventMap.hasEvent(eventId));
        int openChestInstructions = 0;
        int chestId = -1;
        for (const EvtInstruction &instruction : engine->_localEventMap.function(eventId)) {
            if (instruction.opcode != EVENT_OpenChest)
                continue;
            ++openChestInstructions;
            chestId = instruction.data.chest_id;
        }
        ASSERT_EQ(openChestInstructions, 1) << "event " << eventId;
        const int expectedChestId = (eventId - firstStorageEvent + 1) % 20;
        EXPECT_EQ(chestId, expectedChestId) << "event " << eventId;
        ASSERT_GE(chestId, 0);
        ASSERT_LT(chestId, chestReferences.size());
        ++chestReferences[chestId];
    }
    for (int chestId = 0; chestId < chestReferences.size(); ++chestId)
        EXPECT_EQ(chestReferences[chestId], 1) << "chest " << chestId;
}

Pointi depositHeldItem(EngineController &game, int chestId, const Item &item, Pointi cell) {
    pParty->setHoldingItem(item);
    ASSERT_EQ(pParty->pPickedItem.itemId, item.itemId);
    const Pointi click = chestTable[vChests[chestId].chestTypeId].inventoryOffset + cell * 32 + Pointi(16, 16);
    game.pressAndReleaseButton(BUTTON_LEFT, click);
    game.tick();
    EXPECT_EQ(pParty->pPickedItem.itemId, ITEM_NULL);
    InventoryEntry stored = vChests[chestId].inventory.find(item.itemId);
    EXPECT_TRUE(stored);
    return stored ? stored.geometry().topLeft() : Pointi(-1, -1);
}

}  // namespace

GAME_TEST(MM7HarmondaleStorage, BoundaryChestPersistsItemsAndArtifactLedger) {
    if (!mm7::mods::allArtifactsStorageEnabled(engine->gameTitle()))
        GTEST_SKIP() << "Set ENROTH_MM7_ALL_ARTIFACTS_STORAGE=1";

    game.startNewGame();
    engine->config->debug.NoDamage.setValue(true);
    game.teleportTo(MAP_CASTLE_HARMONDALE, Vec3f(), 0);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_HARMONDALE);
    // The live MM7 map table names Castle Harmondale d29.blv; d05 is not this area in the shipped data.
    ASSERT_EQ(pMapTable->pInfos[MAP_CASTLE_HARMONDALE].fileName, std::string_view("d29.blv"));
    ASSERT_EQ(vChests.size(), 20);
    assertStorageEventContract();

    // Event 195 is the authored boundary mapping to chest 0. Use the production chest operation and real chest UI
    // for deposits; a direct event-face click would add geometry-dependent setup without changing this transaction.
    for (int attempt = 0; attempt < 2 && current_screen_type != SCREEN_CHEST; ++attempt) {
        Chest::open(boundaryChestId, Pid());
        game.tick();
    }
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    ASSERT_EQ(pGUIWindow_CurrentChest->chestId(), boundaryChestId);
    vChests[boundaryChestId].inventory.clear();

    Item artifact(storedArtifact);
    artifact.SetIdentified();
    Item potion(storedOrdinaryItem);
    potion.potionPower = 37;
    potion.SetIdentified();
    potion.SetStolen();

    const Pointi artifactCell = depositHeldItem(game, boundaryChestId, artifact, Pointi(0, 0));
    ASSERT_TRUE(pParty->isArtifactCollected[storedArtifact]);
    const Pointi potionCell = depositHeldItem(game, boundaryChestId, potion, Pointi(3, 0));
    ASSERT_NE(artifactCell, potionCell);
    ASSERT_EQ(vChests[boundaryChestId].inventory.size(), 2);

    Chest_MM7 expectedChest = {};
    snapshot(vChests[boundaryChestId], &expectedChest);
    const unsigned int resetDays = pMapTable->pInfos[MAP_CASTLE_HARMONDALE].respawnIntervalDays;
    ASSERT_GT(resetDays, 0u);

    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
    const Time beforeAdvance = pParty->GetPlayingTime();
    pParty->GetPlayingTime() += Duration::fromDays(resetDays + 1);
    const Time savedAt = pParty->GetPlayingTime();
    ASSERT_GE(savedAt, beforeAdvance + Duration::fromDays(resetDays + 1));
    Blob checkpoint = game.saveGame();

    // Prove the following observations come from the save/delta roundtrip rather than surviving process memory.
    vChests[boundaryChestId].inventory.clear();
    pParty->isArtifactCollected[storedArtifact] = false;
    ASSERT_FALSE(vChests[boundaryChestId].inventory.find(storedArtifact));
    ASSERT_FALSE(pParty->isArtifactCollected[storedArtifact]);

    game.loadGame(checkpoint);
    ASSERT_EQ(engine->_currentLoadedMapId, MAP_CASTLE_HARMONDALE);
    EXPECT_GE(pParty->GetPlayingTime(), savedAt);
    EXPECT_TRUE(pParty->isArtifactCollected[storedArtifact]);

    const InventoryEntry restoredArtifact = vChests[boundaryChestId].inventory.find(storedArtifact);
    const InventoryEntry restoredPotion = vChests[boundaryChestId].inventory.find(storedOrdinaryItem);
    ASSERT_TRUE(restoredArtifact);
    ASSERT_TRUE(restoredPotion);
    EXPECT_EQ(restoredArtifact.geometry().topLeft(), artifactCell);
    EXPECT_EQ(restoredPotion.geometry().topLeft(), potionCell);
    EXPECT_EQ(restoredPotion->potionPower, 37);
    EXPECT_TRUE(restoredPotion->IsIdentified());
    EXPECT_TRUE(restoredPotion->IsStolen());

    Chest_MM7 restoredChest = {};
    snapshot(vChests[boundaryChestId], &restoredChest);
    EXPECT_EQ(std::memcmp(&restoredChest, &expectedChest, sizeof(Chest_MM7)), 0);
}
