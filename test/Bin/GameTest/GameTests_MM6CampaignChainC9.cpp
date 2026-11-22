#include <algorithm>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Application/GameConfig.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"

#include "MM6CampaignChain.h"

static constexpr ItemId blaster = static_cast<ItemId>(64);
static constexpr ItemId blasterRifle = static_cast<ItemId>(65);
static constexpr ItemId ritualOfTheVoid = static_cast<ItemId>(544);
static constexpr ItemId sanctumKey = static_cast<ItemId>(570);
static constexpr int reactorActor = 0;
static constexpr int demonQueenActor = 1;

static std::vector<int> hiveBits() {
    // The set ones of the reactor destroyed, Archibald released, the Demon Queen killed and the escape with the Ritual.
    std::vector<int> result;
    for (int bit : {180, 198, 202, 237})
        if (pParty->_questBits[static_cast<QuestBit>(bit)])
            result.push_back(bit);
    return result;
}

static std::vector<ItemId> mainHands() {
    std::vector<ItemId> result;
    for (const Character &character : pParty->pCharacters) {
        InventoryConstEntry weapon = character.inventory.entry(ITEM_SLOT_MAIN_HAND);
        result.push_back(weapon ? weapon->itemId : ITEM_NULL);
    }
    return result;
}

static std::vector<DoorState> sanctumDoor(MapId hive) {
    std::vector<DoorState> result;
    if (engine->_currentLoadedMapId == hive)
        result.push_back(std::ranges::find(pIndoor->doors, 6u, &BLVDoor::doorId)->state);
    return result;
}

static void clickBackpackItem(EngineController &game, int characterIndex, ItemId itemId) {
    InventoryEntry entry = pParty->pCharacters[characterIndex].inventory.find(itemId);
    ASSERT_TRUE(entry);
    game.pressAndReleaseButton(BUTTON_LEFT, Pointi(14, 17) + entry.geometry().topLeft() * 32 + Pointi(16, 16));
    game.tick();
}

static void equipBlaster(EngineController &game, int characterIndex, ItemId weapon) {
    // The first character carries the blasters. It hands one over through the character's portrait, and the character
    // wears it.
    if (characterIndex != 0) {
        game.goToInventory(0);
        clickBackpackItem(game, 0, weapon);
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();
        game.pressGuiButton(fmt::format("Game_Character{}", characterIndex + 1));
        game.tick();
    }
    game.goToInventory(characterIndex);
    clickBackpackItem(game, characterIndex, weapon);
    game.pressAndReleaseButton(BUTTON_LEFT, {520, 150}); // The paperdoll.
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
}

static void openSanctum(EngineController &game) {
    // Chest 2 holds the Hive Sanctum Key. The sanctum door reads the active character's items, so the first character,
    // who carries the key, is selected first.
    clickMM6CampaignChainIndoorEventFace(game, 42);
    takeMM6CampaignChainChestItem(game, 2, sanctumKey);
    if (!pParty->hasActiveCharacter() || pParty->activeCharacterIndex() != 0) {
        game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_1);
        game.tick();
    }
    useMM6CampaignChainIndoorEventFace(game, 6);
}

static void leaveThroughObelisk(EngineController &game) {
    // The obelisk only answers a character who has recovered from the fight.
    for (int frame = 0; frame < 100 && !pParty->hasActiveCharacter(); frame++)
        game.tick();
    useMM6CampaignChainIndoorEventFace(game, 60);
    game.tick(); // The ending opens on the next message pass.
}

static void closeEnding(EngineController &game) {
    // The ending screen ignores keys for its first 5 seconds. The first key after that shows the farewell box, the
    // next one closes it, and the party goes on in New Sorpigal.
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(50);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

GAME_TEST(MM6CampaignChain, DISABLED_Chapter9) {
    // Fixtures: C1's party strength, and the party takes no damage inside the hive. Teleports to the hive's door on
    // outa1, and inside the hive to chest 2, the sanctum door, next to the Demon Queen and the reactor, and to the exit
    // obelisk.
    assertMM6CampaignChainProfile();
    loadMM6CampaignChainSlot(game, 8);
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    const MapId outa1 = pMapTable->GetMapInfo("outa1.odm");
    const MapId hive = pMapTable->GetMapInfo("hive.blv");
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    ASSERT_EQ(engine->_currentLoadedMapId, outd3);

    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto bitsTape = tapes.custom([] { return hiveBits(); });
    auto savedEnrothTape = tapes.custom([] { return static_cast<bool>(pParty->pCharacters[0]._achievedAwardsBits[static_cast<AwardId>(36)]); });
    auto handsTape = tapes.custom([] { return mainHands(); });
    auto keyTape = tapes.hasItem(sanctumKey);
    auto sanctumDoorTape = tapes.custom([=] { return sanctumDoor(hive); });
    auto ritualTape = tapes.hasItem(ritualOfTheVoid);
    auto positionTape = tapes.custom([] { return pParty->pos.xy(); });
    auto certificateTape = tapes.custom([] { return ufs->exists("MM6_Win.Pcx"); });
    auto messageBoxesTape = tapes.messageBoxes();
    auto hpTape = charTapes.hps();
    test.startTaping();

    equipBlaster(game, 1, blaster);
    equipBlaster(game, 2, blaster);
    equipBlaster(game, 3, blaster);
    equipBlaster(game, 0, blasterRifle);
    useMM6CampaignChainEventFace(game, outa1, 90); // The hive.
    confirmMM6CampaignChainTransition(game);
    ASSERT_EQ(engine->_currentLoadedMapId, hive);
    ASSERT_EQ(pActors[reactorActor].monsterInfo.id, static_cast<MonsterId>(173));
    ASSERT_EQ(pActors[demonQueenActor].monsterInfo.id, static_cast<MonsterId>(172));
    engine->config->debug.NoDamage.setValue(true);
    openSanctum(game);
    killMM6CampaignChainActor(game, demonQueenActor);
    killMM6CampaignChainActor(game, reactorActor);
    leaveThroughObelisk(game);
    ASSERT_EQ(screenTape.back(), SCREEN_GAMEOVER_WINDOW);
    engine->config->debug.NoDamage.setValue(false);
    closeEnding(game);

    EXPECT_EQ(mapTape, tape(outd3, outa1, hive, oute3));
    EXPECT_CONTAINS(screenTape, SCREEN_GAMEOVER_WINDOW);
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_EQ(handsTape, tape(std::vector{ITEM_NULL, ITEM_NULL, ITEM_NULL, ITEM_NULL}, std::vector{ITEM_NULL, blaster, ITEM_NULL, ITEM_NULL},
                              std::vector{ITEM_NULL, blaster, blaster, ITEM_NULL}, std::vector{ITEM_NULL, blaster, blaster, blaster},
                              std::vector{blasterRifle, blaster, blaster, blaster}));
    EXPECT_EQ(keyTape, tape(false, true, false)); // The sanctum door takes its key.
    EXPECT_EQ(sanctumDoorTape, tape(std::vector<DoorState>(), std::vector{DOOR_OPEN}, std::vector{DOOR_CLOSING}, std::vector{DOOR_CLOSED},
                                    std::vector{DOOR_OPEN}, std::vector<DoorState>())); // Open is the moved state, DOOR_CLOSED. The frame the party leaves on reads it unmoved.
    EXPECT_EQ(bitsTape, tape(std::vector<int>{198}, std::vector<int>{198, 202}, std::vector<int>{180, 198, 202}, std::vector<int>{180, 202, 237}));
    EXPECT_CONTAINS(positionTape, Vec2f(3328, 25920)); // The reactor puts the party at the hive exit.
    EXPECT_EQ(ritualTape, tape(true, false)); // The obelisk takes the Ritual.
    EXPECT_EQ(savedEnrothTape, tape(false, true)); // Award 36, Destroyed the Hive and Saved Enroth.
    EXPECT_EQ(certificateTape, tape(false, true));
    auto messageBoxes = messageBoxesTape.flatten();
    ASSERT_EQ(messageBoxes.size(), 1);
    EXPECT_TRUE(messageBoxes.front().starts_with("Congratulations Adventurer."));
    for (const auto &hps : hpTape)
        for (int hp : hps)
            EXPECT_GT(hp, 0);
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveMM6CampaignChainSlot(game, 9);
}
