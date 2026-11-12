#include <algorithm>
#include <initializer_list>
#include <string>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Localization.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

#include "MM6CampaignChain.h"

static std::vector<int> councilBits() {
    // The set ones of the Hourglass taken, Gharik's key found, Albert Newton's quest, the Hourglass carried, the key
    // carried and the Forge unlocked.
    std::vector<int> result;
    for (int bit : {5, 11, 110, 183, 199, 376})
        if (pParty->_questBits[static_cast<QuestBit>(bit)])
            result.push_back(bit);
    return result;
}

static std::vector<int> councilAwards() {
    // The six council votes, then the letter delivered to Wilbur Humphrey.
    std::vector<int> result;
    for (int award : {2, 3, 4, 5, 6, 7, 58})
        if (pParty->pCharacters[0]._achievedAwardsBits[static_cast<AwardId>(award)])
            result.push_back(award);
    return result;
}

static std::vector<DoorState> doorStates(MapId map, std::initializer_list<int> doorIds) {
    // The doors' states while the party is on the map.
    std::vector<DoorState> result;
    if (engine->_currentLoadedMapId == map)
        for (int doorId : doorIds)
            result.push_back(std::ranges::find(pIndoor->doors, doorId, &BLVDoor::doorId)->state);
    return result;
}

static void confirmTransition(EngineController &game) {
    ASSERT_EQ(current_screen_type, SCREEN_INPUT_BLV);
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();
}

static void talkToNewton(EngineController &game, int topic) {
    enterMM6CampaignChainCastle(game, pMapTable->GetMapInfo("oute2.odm"), 30);
    talkToMM6CampaignChainNpc(game, 5);
    ASSERT_TRUE(selectMM6CampaignChainTopic(game, topic));
    leaveMM6CampaignChainHouse(game);
}

GAME_TEST(MM6CampaignChain, DISABLED_Chapter2) {
    // Fixtures: C1's party strength. Teleports to the outd3 dock, to the doors of Silver Helm Outpost and of Gharik's
    // Forge on its island, back to Castle Newton, and inside both dungeons to the Forge's key lever, the chests and the
    // ways out.
    assertMM6CampaignChainProfile();
    loadMM6CampaignChainSlot(game, 1);
    const MapId outd1 = pMapTable->GetMapInfo("outd1.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    const MapId oute2 = pMapTable->GetMapInfo("oute2.odm");
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId d07 = pMapTable->GetMapInfo("d07.blv");
    const MapId d18 = pMapTable->GetMapInfo("d18.blv");
    ASSERT_EQ(engine->_currentLoadedMapId, outd1);
    ASSERT_EQ(councilAwards(), std::vector<int>({2, 6, 58}));

    auto mapTape = tapes.map();
    auto bitsTape = tapes.custom([] { return councilBits(); });
    auto awardsTape = tapes.custom([] { return councilAwards(); });
    auto keyTape = tapes.hasItem(static_cast<ItemId>(487));
    auto hourglassTape = tapes.hasItem(static_cast<ItemId>(433));
    auto doorTape = tapes.custom([d18] { return doorStates(d18, {30}); });
    auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
    auto hpTape = charTapes.hps();
    test.startTaping();

    game.teleportTo(outd3, Vec3f(21141, 8191, 192), 0); // The outd3 dock, facing the ship.
    game.pointMouseAtDecoration(127); // The ship.
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
    const std::string route = localization->format(LSTR_D_DAYS_TO_S, 2, pMapTable->pInfos[oute2].name);
    auto option = std::ranges::find(window_SpeakInHouse->vButtons, route, &GUIButton::label);
    ASSERT_NE(option, window_SpeakInHouse->vButtons.end()); // The ship sails to the Misty Islands on the day C1 ends.
    game.pressGuiButton((*option)->id);
    game.tick(27); // The ship leaves two and a half seconds after the party pays, then the house closes.
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(engine->_currentLoadedMapId, oute2);
    talkToNewton(game, 51); // Albert Newton asks for the Hourglass of Time.

    useMM6CampaignChainEventFace(game, oute2, 90); // Silver Helm Outpost.
    confirmTransition(game);
    ASSERT_EQ(engine->_currentLoadedMapId, d07);
    useMM6CampaignChainIndoorEventFace(game, 16);
    takeMM6CampaignChainChestItem(game, 1, static_cast<ItemId>(487));
    useMM6CampaignChainIndoorEventFace(game, 20); // The way out.
    confirmTransition(game);

    useMM6CampaignChainEventFace(game, oute3, 103); // Gharik's Forge.
    confirmTransition(game);
    ASSERT_EQ(engine->_currentLoadedMapId, d18);
    useMM6CampaignChainIndoorEventFace(game, 16); // The lever takes the key.
    useMM6CampaignChainIndoorEventFace(game, 48);
    takeMM6CampaignChainChestItem(game, 5, static_cast<ItemId>(433));
    useMM6CampaignChainIndoorEventFace(game, 90); // The way out.
    confirmTransition(game);

    talkToNewton(game, 52); // Albert Newton takes the Hourglass.

    EXPECT_EQ(mapTape, tape(outd1, outd3, oute2, d07, oute2, oute3, d18, oute3, oute2));
    EXPECT_EQ(bitsTape, tape(std::vector<int>(), std::vector<int>{110}, std::vector<int>{11, 110, 199}, std::vector<int>{11, 110, 376},
                             std::vector<int>{5, 11, 110, 183, 376}, std::vector<int>{5, 11, 376}));
    EXPECT_EQ(awardsTape, tape(std::vector<int>{2, 6, 58}, std::vector<int>{2, 3, 6, 58}));
    EXPECT_EQ(keyTape, tape(false, true, false));
    EXPECT_EQ(hourglassTape, tape(false, true, false));
    EXPECT_CONTAINS(doorTape, std::vector<DoorState>{DOOR_CLOSING}); // The door by the lever slides away.
    EXPECT_EQ(reputationTape, tape(0, 200)); // Albert Newton's thanks.
    for (const auto &hps : hpTape)
        for (int hp : hps)
            EXPECT_GT(hp, 0);
    ASSERT_FALSE(::testing::Test::HasFailure());

    saveMM6CampaignChainSlot(game, 2);
}
