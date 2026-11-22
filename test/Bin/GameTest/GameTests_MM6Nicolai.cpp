#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"

#include "GUI/GUIWindow.h"

#include "MM6CampaignChain.h"

static constexpr int nicolai = 13;

static void startNewGameThroughInput(EngineController &game) {
    // Each character takes its first three optional skills and up to three points in every attribute, then the party
    // enters the first map.
    constexpr PlatformKey characterKeys[] = {PlatformKey::KEY_DIGIT_1, PlatformKey::KEY_DIGIT_2, PlatformKey::KEY_DIGIT_3,
                                             PlatformKey::KEY_DIGIT_4};
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    for (PlatformKey key : characterKeys) {
        game.pressAndReleaseKey(key);
        game.tick();
        for (int skill = 0; skill < 3; skill++) {
            game.pressGuiButton(fmt::format("PartyCreation_Skill{}", skill));
            game.tick();
        }
        for (int attribute = 0; attribute < 7; attribute++) {
            for (int point = 0; point < 3; point++) {
                game.pressAndReleaseKey(PlatformKey::KEY_ADD);
                game.tick();
            }
            game.pressAndReleaseKey(PlatformKey::KEY_DOWN);
            game.tick();
        }
    }
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
}

static void hireNicolai(EngineController &game) {
    // In the Castle Ironfist throne room, Nicolai's Boredom topic sets the Entertain Nicolai bit and he joins the party.
    enterMM6CampaignChainCastle(game, pMapTable->GetMapInfo("outd3.odm"), 43);
    talkToMM6CampaignChainNpc(game, nicolai);
    ASSERT_TRUE(selectMM6CampaignChainTopic(game, 18));
    leaveMM6CampaignChainHouse(game);
}

static void takeCoachFromNewSorpigal(EngineController &game) {
    // Space at the door of the New Sorpigal stable, then its only route to outd3.
    game.teleportTo(pMapTable->GetMapInfo("oute3.odm"), Vec3f(-10346, -9472, 161), 180);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    game.pressGuiButton("HouseDialogue_Option0");
    game.tick(17); // The coach leaves a second and a half after the party pays, then the house closes.
    game.skipLoadingScreen();
    game.tick();
}

static int followers() {
    FlatHirelings hirelings;
    hirelings.Prepare();
    return static_cast<int>(hirelings.Size());
}

static std::tuple<int, unsigned> nicolaiHouseAndTopic() {
    return std::tuple(std::to_underlying(pNPCStats->pNPCData[nicolai].house), pNPCStats->pNPCData[nicolai].dialogue_1_evt_id);
}

static std::vector<int> nicolaiBits() {
    // The set ones of Entertain Nicolai and Find and return Prince Nicolai.
    std::vector<int> result;
    for (int bit : {90, 95})
        if (pParty->_questBits[static_cast<QuestBit>(bit)])
            result.push_back(bit);
    return result;
}

GAME_TEST(MM6Nicolai, DisappearsAfterCoach) {
    // Prince Nicolai stayed with the party through a coach trip, where MM6 has him disappear after the trip's rest.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    auto partyTape = tapes.custom([] { return std::tuple(engine->_currentLoadedMapId, followers()); });
    auto bitsTape = tapes.custom([] { return nicolaiBits(); });
    auto nicolaiTape = tapes.custom([] { return nicolaiHouseAndTopic(); });
    auto screenTape = tapes.screen();
    auto textTape = tapes.allGUIWindowsText();
    test.startTaping();

    hireNicolai(game);
    takeCoachFromNewSorpigal(game);

    EXPECT_EQ(partyTape, tape(std::tuple(oute3, 0), std::tuple(outd3, 0), std::tuple(outd3, 1), std::tuple(oute3, 1), std::tuple(outd3, 1),
                              std::tuple(outd3, 0)));
    EXPECT_EQ(bitsTape, tape(std::vector<int>{}, std::vector<int>{90}, std::vector<int>{95}));
    EXPECT_EQ(nicolaiTape, tape(std::tuple(154, 18u), std::tuple(154, 19u), std::tuple(166, 21u)));
    EXPECT_EQ(screenTape.back(), SCREEN_BRANCHLESS_NPC_DIALOG);
    EXPECT_CONTAINS(textTape.flatten(), std::string("It seems that Prince Nicolai disappeared while you\nwere resting."));
}

GAME_TEST(MM6Nicolai, DisappearsAfterRestScreen) {
    // Prince Nicolai stayed with the party when the rest screen closed, where MM6 has him disappear, even when the
    // party didn't rest.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    hireNicolai(game);
    auto partyTape = tapes.custom([] { return followers(); });
    auto bitsTape = tapes.custom([] { return nicolaiBits(); });
    auto nicolaiTape = tapes.custom([] { return nicolaiHouseAndTopic(); });
    auto screenTape = tapes.screen();
    auto timeTape = tapes.time();
    auto textTape = tapes.allGUIWindowsText();
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();

    EXPECT_EQ(partyTape, tape(1, 0));
    EXPECT_EQ(bitsTape, tape(std::vector<int>{90}, std::vector<int>{95}));
    EXPECT_EQ(nicolaiTape, tape(std::tuple(154, 19u), std::tuple(166, 21u)));
    EXPECT_EQ(screenTape, tape(SCREEN_REST, SCREEN_BRANCHLESS_NPC_DIALOG));
    EXPECT_EQ(timeTape.size(), 1);
    EXPECT_CONTAINS(textTape.flatten(), std::string("It seems that Prince Nicolai disappeared while you\nwere resting."));
}

GAME_TEST(MM6Nicolai, DisappearsAfterBorderTrip) {
    // Prince Nicolai stayed with the party through a border trip, where MM6 has him disappear after the trip's rest.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    hireNicolai(game);
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    game.teleportTo(outd3, Vec3f(22450, -10000, 164), 0); // Next to the east edge, facing it.
    auto partyTape = tapes.custom([] { return std::tuple(engine->_currentLoadedMapId, followers()); });
    auto bitsTape = tapes.custom([] { return nicolaiBits(); });
    auto screenTape = tapes.screen();
    test.startTaping();

    game.pressKey(PlatformKey::KEY_UP);
    game.tick(2); // The party walks off the edge and the travel dialogue opens.
    game.releaseKey(PlatformKey::KEY_UP);
    ASSERT_EQ(screenTape.back(), SCREEN_CHANGE_LOCATION);
    game.pressGuiButton("Transition_Yes");
    game.tick();
    game.skipLoadingScreen();
    game.tick();

    EXPECT_EQ(partyTape, tape(std::tuple(outd3, 1), std::tuple(oute3, 1), std::tuple(oute3, 0)));
    EXPECT_EQ(bitsTape, tape(std::vector<int>{90}, std::vector<int>{95}));
    EXPECT_EQ(screenTape.back(), SCREEN_BRANCHLESS_NPC_DIALOG);
}
