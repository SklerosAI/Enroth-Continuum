#include <algorithm>
#include <memory>
#include <ranges>
#include <set>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Mods/PrivateMods.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Objects/ItemEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/Snapshots/EntitySnapshots.h"
#include "Engine/Tables/ItemTable.h"
#include "Engine/Tables/ChestTable.h"

#include "GUI/GUIWindow.h"
#include "GUI/GUIButton.h"
#include "GUI/UI/UIChest.h"

namespace {

bool allArtifactsEnabled() {
    return mm7::mods::allArtifactsStorageEnabled(engine->gameTitle());
}

}  // namespace

GAME_TEST(MM7AllArtifactsEngine, DisabledModeLeavesCollectionStateUntouched) {
    if (allArtifactsEnabled())
        GTEST_SKIP() << "Run without ENROTH_MM7_ALL_ARTIFACTS_STORAGE";
    game.startNewGame();

    constexpr ItemId target = ITEM_ARTIFACT_PUCK;
    pParty->setHoldingItem(Item(target));
    EXPECT_FALSE(pParty->artifactCollectionInitialized);
    EXPECT_FALSE(pParty->isArtifactCollected[target]);
    pParty->takeHoldingItem();
}

GAME_TEST(MM7ChestReroll, KeepsAuthoredChestAndStopsAfterClaim) {
    if (!mm7::mods::chestRerollEnabled(engine->gameTitle()))
        GTEST_SKIP() << "Set ENROTH_MM7_CHEST_REROLL=1";
    game.startNewGame();

    constexpr int chestId = 0;
    Chest &chest = vChests[chestId];
    chest.chestTypeId = 0;
    chest.inventory = ChestInventory(chestTable[0].size);
    chest.inventory.stash(Item(ITEM_RANDOM_LEVEL_4));
    Item fixed(ITEM_CRUDE_LONGSWORD);
    fixed.SetIdentified();
    chest.inventory.stash(fixed);
    GenerateItemsInChest();
    Chest::PlaceItems(chestId);
    ASSERT_TRUE(Chest::canReroll(chestId));
    const ChestFlags originalFlags = chest.flags;

    auto wire = std::make_unique<Chest_MM7>();
    snapshot(chest, wire.get());
    reconstruct(*wire, &chest, tags::context<int>(chestId));
    ASSERT_TRUE(Chest::canReroll(chestId));
    GenerateItemsInChest(); // A normal map reload must not generate the retained marker again.
    ASSERT_TRUE(Chest::canReroll(chestId));

    pGUIWindow_CurrentChest = std::make_unique<GUIWindow_Chest>(chestId);
    ASSERT_EQ(current_screen_type, SCREEN_CHEST);
    EXPECT_TRUE(std::ranges::any_of(pGUIWindow_CurrentChest->vButtons,
                                    [](const GUIButton *button) { return button->id == "Chest_Reroll"; }));
    bool sawMarker = false;
    for (int attempt = 0; attempt < 3; ++attempt) {
        ASSERT_TRUE(Chest::reroll(chestId));
        EXPECT_EQ(chest.chestTypeId, 0);
        EXPECT_EQ(chest.flags, originalFlags);
        sawMarker = false;
        for (InventoryConstEntry entry : chest.inventory.entries())
            if (entry->chestRerollSource == 255) {
                EXPECT_EQ(entry->itemId, ITEM_RANDOM_LEVEL_4);
                EXPECT_EQ(entry.zone(), INVENTORY_ZONE_STASH);
                sawMarker = true;
            }
        EXPECT_TRUE(sawMarker);
        InventoryConstEntry authored = chest.inventory.find(ITEM_CRUDE_LONGSWORD);
        ASSERT_TRUE(authored);
        EXPECT_TRUE(authored->IsIdentified());
        EXPECT_EQ(authored->chestRerollSource, 0);
    }

    Chest::GrabItem(false);
    EXPECT_FALSE(Chest::canReroll(chestId));
    EXPECT_FALSE(Chest::reroll(chestId));
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
}

GAME_TEST(MM7AllArtifactsEngine, CollectsAllTwentyNineAndIgnoresVanillaCap) {
    if (!allArtifactsEnabled())
        GTEST_SKIP() << "Set ENROTH_MM7_ALL_ARTIFACTS_STORAGE=1";
    game.startNewGame();
    engine->config->gameplay.ArtifactLimit.setValue(1);

    std::set<ItemId> generatedIds;
    for (int i = 0; i < 29; ++i) {
        Item generated;
        ASSERT_TRUE(generated.GenerateArtifact());
        ASSERT_TRUE(isSpawnableArtifact(generated.itemId));
        EXPECT_TRUE(pParty->isArtifactGenerated[generated.itemId]);
        EXPECT_FALSE(pParty->isArtifactCollected[generated.itemId]);
        EXPECT_TRUE(generatedIds.insert(generated.itemId).second);

        pParty->setHoldingItem(generated);
        EXPECT_TRUE(pParty->isArtifactCollected[generated.itemId]);
        pParty->takeHoldingItem();
    }
    EXPECT_EQ(generatedIds.size(), 29);

    Item exhausted;
    EXPECT_FALSE(exhausted.GenerateArtifact());
    EXPECT_EQ(exhausted.itemId, ITEM_NULL);
}

GAME_TEST(MM7AllArtifactsEngine, NativeGeneratedFlagDoesNotExcludeUntilAcquisition) {
    if (!allArtifactsEnabled())
        GTEST_SKIP() << "Set ENROTH_MM7_ALL_ARTIFACTS_STORAGE=1";
    game.startNewGame();
    pParty->initializeCollectedArtifactsFromOwnedItems();
    pParty->isArtifactCollected.fill(true);

    constexpr ItemId target = ITEM_ARTIFACT_PUCK;
    pParty->isArtifactCollected[target] = false;
    pParty->isArtifactGenerated[target] = true;
    engine->config->gameplay.ArtifactLimit.setValue(1);

    Item generated;
    ASSERT_TRUE(generated.GenerateArtifact());
    EXPECT_EQ(generated.itemId, target);
    EXPECT_FALSE(pParty->isArtifactCollected[target]);

    Item acquired(target);
    ASSERT_TRUE(pParty->addItemToParty(&acquired, true));
    EXPECT_EQ(acquired.itemId, ITEM_NULL);
    EXPECT_TRUE(pParty->isArtifactCollected[target]);

    Item exhausted;
    EXPECT_FALSE(exhausted.GenerateArtifact());
}

GAME_TEST(MM7AllArtifactsEngine, TierSixUsesUncollectedPoolAndExhaustsSafely) {
    if (!allArtifactsEnabled())
        GTEST_SKIP() << "Set ENROTH_MM7_ALL_ARTIFACTS_STORAGE=1";
    game.startNewGame();
    pParty->initializeCollectedArtifactsFromOwnedItems();
    pParty->isArtifactCollected.fill(true);

    constexpr ItemId target = ITEM_ARTIFACT_IRON_FEATHER;
    pParty->isArtifactCollected[target] = false;
    pParty->isArtifactGenerated[target] = true;
    engine->config->gameplay.ArtifactLimit.setValue(1);

    bool generatedTarget = false;
    for (int attempt = 0; attempt < 500 && !generatedTarget; ++attempt) {
        Item item;
        pItemTable->generateItem(ITEM_TREASURE_LEVEL_6, RANDOM_ITEM_ANY, &item);
        generatedTarget = item.itemId == target;
    }
    EXPECT_TRUE(generatedTarget);
    EXPECT_FALSE(pParty->isArtifactCollected[target]);

    pParty->isArtifactCollected[target] = true;
    for (int attempt = 0; attempt < 100; ++attempt) {
        Item item;
        pItemTable->generateItem(ITEM_TREASURE_LEVEL_6, RANDOM_ITEM_ANY, &item);
        EXPECT_FALSE(isSpawnableArtifact(item.itemId));
    }
}

GAME_TEST(MM7AllArtifactsEngine, ChestGrabMarksDirectInventoryAcquisition) {
    if (!allArtifactsEnabled())
        GTEST_SKIP() << "Set ENROTH_MM7_ALL_ARTIFACTS_STORAGE=1";
    game.startNewGame();
    pParty->initializeCollectedArtifactsFromOwnedItems();
    for (Character &character : pParty->pCharacters)
        character.inventory.clear();

    constexpr int chestId = 0;
    constexpr ItemId target = ITEM_ARTIFACT_WALLACE;
    ASSERT_LT(chestId, vChests.size());
    // This also models a legacy artifact left in storage: migration cannot see it, but withdrawal can.
    vChests[chestId].inventory.clear();
    vChests[chestId].inventory.add(Item(target));
    EXPECT_FALSE(pParty->isArtifactCollected[target]);

    pGUIWindow_CurrentChest = std::make_unique<GUIWindow_Chest>(chestId);
    EXPECT_FALSE(pParty->isArtifactCollected[target]);
    Chest::GrabItem(false);
    EXPECT_TRUE(pParty->isArtifactCollected[target]);
    EXPECT_TRUE(pParty->activeCharacter().inventory.find(target));

    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
}

GAME_TEST(MM7AllArtifactsEngine, LegacyOwnershipMigratesAndNewLedgerRoundTrips) {
    if (!allArtifactsEnabled())
        GTEST_SKIP() << "Set ENROTH_MM7_ALL_ARTIFACTS_STORAGE=1";
    game.startNewGame();
    for (Character &character : pParty->pCharacters)
        character.inventory.clear();

    constexpr ItemId backpackArtifact = ITEM_ARTIFACT_GOVERNORS_ARMOR;
    constexpr ItemId equippedArtifact = ITEM_ARTIFACT_GHOULSBANE;
    constexpr ItemId cursorArtifact = ITEM_RELIC_MASH;
    constexpr ItemId generatedOnlyArtifact = ITEM_RELIC_ETHRICS_STAFF;
    pParty->pCharacters[0].inventory.add(Item(backpackArtifact));
    pParty->pCharacters[1].inventory.equip(ITEM_SLOT_MAIN_HAND, Item(equippedArtifact));
    pParty->pPickedItem = Item(cursorArtifact);
    pParty->isArtifactGenerated[generatedOnlyArtifact] = true;
    pParty->isArtifactCollected.fill(false);
    pParty->artifactCollectionInitialized = false;
    pParty->artifactCollectionVersion = 0;

    auto legacyWire = std::make_unique<Party_MM7>();
    snapshot(*pParty, legacyWire.get());
    EXPECT_TRUE(std::ranges::all_of(legacyWire->field_7d7, [](char value) { return value == 0; }));

    auto migrated = std::make_unique<Party>();
    reconstruct(*legacyWire, migrated.get());
    EXPECT_TRUE(migrated->artifactCollectionInitialized);
    EXPECT_EQ(migrated->artifactCollectionVersion, 1);
    EXPECT_TRUE(migrated->isArtifactCollected[backpackArtifact]);
    EXPECT_TRUE(migrated->isArtifactCollected[equippedArtifact]);
    EXPECT_TRUE(migrated->isArtifactCollected[cursorArtifact]);
    // Legacy generated flags cannot prove acquisition, and stored/world artifacts are discovered on withdrawal.
    EXPECT_FALSE(migrated->isArtifactCollected[generatedOnlyArtifact]);

    auto currentWire = std::make_unique<Party_MM7>();
    snapshot(*migrated, currentWire.get());
    EXPECT_FALSE(std::ranges::all_of(currentWire->field_7d7, [](char value) { return value == 0; }));

    auto restored = std::make_unique<Party>();
    reconstruct(*currentWire, restored.get());
    EXPECT_TRUE(restored->artifactCollectionInitialized);
    EXPECT_EQ(restored->artifactCollectionVersion, 1);
    for (ItemId itemId : allSpawnableArtifacts())
        EXPECT_EQ(restored->isArtifactCollected[itemId], migrated->isArtifactCollected[itemId]);
}
