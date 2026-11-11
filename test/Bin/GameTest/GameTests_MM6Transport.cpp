#include <string>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Localization.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/mm7_data.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIHouses.h"

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

static std::vector<std::string> houseOptionLabels() {
    // The menu lines the house screen shows, without the hidden ones.
    std::vector<std::string> result;
    if (window_SpeakInHouse)
        for (GUIButton *button : window_SpeakInHouse->vButtons)
            if (button->id.contains("Dialogue_Option") && button->rect.h > 0)
                result.push_back(button->label);
    return result;
}

static std::string routeLabel(int days, const char *map) {
    return localization->format(LSTR_D_DAYS_TO_S, days, pMapTable->pInfos[pMapTable->GetMapInfo(map)].name);
}

static std::vector<int> priceFixingBits() {
    // Loretta Fleise's price fixing bits that are set: taken on, all done, and one per stable.
    std::vector<int> result;
    for (int bit : {116, 117, 147, 148, 149, 150, 151, 152, 153, 154, 155})
        if (pParty->_questBits[static_cast<QuestBit>(bit)])
            result.push_back(bit);
    return result;
}

static void enterNewSorpigalStable(EngineController &game) {
    game.teleportTo(engine->_currentLoadedMapId, Vec3f(-10346, -9472, 161), 180);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
}

static void takeCoachFromNewSorpigal(EngineController &game) {
    // Space at the door of the New Sorpigal stable, then its only route to outd3.
    enterNewSorpigalStable(game);
    game.pressGuiButton("HouseDialogue_Option0");
    game.tick(17); // The coach leaves a second and a half after the party pays, then the house closes.
    game.skipLoadingScreen();
    game.tick();
}

static void boardShip(EngineController &game, Vec3f dock, int yaw, int decoration) {
    // Stands on the dock and clicks the ship, whose script opens its boat house.
    game.teleportTo(engine->_currentLoadedMapId, dock, yaw);
    game.pointMouseAtDecoration(decoration);
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();
}

static void sail(EngineController &game, int slot) {
    game.pressGuiButton(fmt::format("HouseDialogue_Option{}", slot));
    game.tick(27); // The ship leaves two and a half seconds after the party pays, then the house closes.
    game.skipLoadingScreen();
    game.tick();
}

GAME_TEST(MM6Transport, StableToNeighbour) {
    // MM6 stables listed no routes, and the party couldn't travel by coach.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto optionsTape = tapes.custom([] { return houseOptionLabels(); });
    auto positionTape = tapes.custom([] { return pParty->pos; });
    auto yawTape = tapes.custom([] { return pParty->_viewYaw; });
    auto goldTape = tapes.gold();
    auto timeTape = tapes.custom([] { return pParty->GetPlayingTime().ticks(); });
    auto foodTape = tapes.food();
    auto portraitTape = charTapes.portrait(0);
    test.startTaping();

    takeCoachFromNewSorpigal(game);

    EXPECT_EQ(mapTape, tape(oute3, outd3));
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_HOUSE, SCREEN_GAME));
    EXPECT_EQ(optionsTape, tape(std::vector<std::string>{}, std::vector<std::string>{routeLabel(2, "outd3.odm")}, std::vector<std::string>{}));
    EXPECT_EQ(positionTape.back(), Vec3f(14317, 2696, 97)); // The route's arrival point.
    EXPECT_EQ(yawTape.back(), 1024);
    EXPECT_EQ(goldTape, tape(200, 175));
    EXPECT_CONTAINS(timeTape.adjacentDeltas(), Duration::fromDays(2).ticks()); // The trip, in one frame.
    EXPECT_EQ(foodTape, tape(7));
    EXPECT_CONTAINS(portraitTape, PORTRAIT_YES); // The coach reaction.
}

GAME_TEST(MM6Transport, BoatToMistAndBack) {
    // MM6 boats listed no routes, so the Misty Islands, which no land touches, couldn't be reached.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    pParty->SetGold(1000); // A new party can't pay for both voyages.
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    const MapId oute2 = pMapTable->GetMapInfo("oute2.odm");
    auto mapTape = tapes.map();
    auto houseTape = tapes.house();
    auto optionsTape = tapes.custom([] { return houseOptionLabels(); });
    auto positionTape = tapes.custom([] { return pParty->pos; });
    auto goldTape = tapes.gold();
    auto timeTape = tapes.custom([] { return pParty->GetPlayingTime().ticks(); });
    auto foodTape = tapes.food();
    test.startTaping();

    takeCoachFromNewSorpigal(game); // Arrives on a Wednesday, when the outd3 ship sails to the Misty Islands.
    boardShip(game, Vec3f(21141, 8191, 192), 0, 127);
    ASSERT_EQ(houseTape.back(), static_cast<HouseId>(58));
    sail(game, 1);
    ASSERT_EQ(mapTape.back(), oute2);
    EXPECT_EQ(positionTape.back(), Vec3f(-4225, -14604, 177));
    boardShip(game, Vec3f(-4225, -14604, 177), 180, 1);
    ASSERT_EQ(houseTape.back(), static_cast<HouseId>(60));
    sail(game, 0);

    EXPECT_EQ(mapTape, tape(oute3, outd3, oute2, outd3));
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(48), HOUSE_INVALID, static_cast<HouseId>(58), HOUSE_INVALID,
                              static_cast<HouseId>(60), HOUSE_INVALID));
    const std::vector<std::string> none;
    EXPECT_EQ(optionsTape, tape(none, std::vector<std::string>{routeLabel(2, "outd3.odm")}, none, std::vector<std::string>{routeLabel(2, "oute2.odm")}, none,
                                std::vector<std::string>{routeLabel(2, "outd3.odm")}, none)); // The stable, a Wednesday at the outd3 ship, a Friday at the oute2 ship.
    EXPECT_EQ(positionTape.back(), Vec3f(21141, 8191, 193)); // Back on the outd3 dock.
    EXPECT_EQ(goldTape, tape(1000, 975, 875, 625));
    EXPECT_EQ(timeTape.adjacentDeltas().count([](int64_t delta) { return delta == Duration::fromDays(2).ticks(); }), 3);
    EXPECT_EQ(foodTape, tape(7));
}

GAME_TEST(MM6Transport, QuestRouteToHermitsIsle) {
    // The outd3 ship never listed its voyage to the Hermit's Isle, which quest bit 168 opens after the traitor step.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    const MapId outa3 = pMapTable->GetMapInfo("outa3.odm");
    takeCoachFromNewSorpigal(game);
    auto mapTape = tapes.map();
    auto houseTape = tapes.house();
    auto optionsTape = tapes.custom([] { return houseOptionLabels(); });
    auto positionTape = tapes.custom([] { return pParty->pos; });
    auto goldTape = tapes.gold();
    auto timeTape = tapes.custom([] { return pParty->GetPlayingTime().ticks(); });
    test.startTaping();

    boardShip(game, Vec3f(21141, 4570, 192), 0, 128);
    game.pressGuiButton("House_Exit");
    game.tick();
    pParty->_questBits.set(static_cast<QuestBit>(168));
    boardShip(game, Vec3f(21141, 4570, 192), 0, 128);
    sail(game, 2);

    EXPECT_EQ(mapTape, tape(outd3, outa3));
    EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(59), HOUSE_INVALID, static_cast<HouseId>(59), HOUSE_INVALID));
    EXPECT_EQ(optionsTape, tape(std::vector<std::string>{}, std::vector<std::string>{routeLabel(21, "outa3.odm")}, std::vector<std::string>{}));
    EXPECT_EQ(positionTape.back(), Vec3f(-18486, -20342, 1));
    EXPECT_EQ(goldTape, tape(175)); // This ship's captain doesn't charge.
    EXPECT_CONTAINS(timeTape.adjacentDeltas(), Duration::fromDays(21).ticks());
}

GAME_TEST(MM6Transport, StableAgreesToFixPrices) {
    // MM6 stables never offered Loretta Fleise's price fixing, so her quest couldn't be finished.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    auto optionsTape = tapes.custom([] { return houseOptionLabels(); });
    auto bitsTape = tapes.custom([] { return priceFixingBits(); });
    auto textTape = tapes.custom([] { return current_npc_text; });
    auto goldTape = tapes.gold();
    test.startTaping();

    enterNewSorpigalStable(game);
    game.pressGuiButton("House_Exit");
    game.tick();
    pParty->_questBits.set(static_cast<QuestBit>(116)); // Loretta Fleise's price fixing topic in outd1 sets it.
    enterNewSorpigalStable(game);
    game.pressGuiButton("HouseDialogue_Option3");
    game.tick();

    const std::vector<std::string> route = {routeLabel(2, "outd3.odm")};
    const std::vector<std::string> routeAndPriceFixing = {routeLabel(2, "outd3.odm"), pNPCTopics[99].pTopic};
    EXPECT_EQ(optionsTape, tape(std::vector<std::string>{}, route, std::vector<std::string>{}, routeAndPriceFixing, route));
    EXPECT_EQ(bitsTape, tape(std::vector<int>{}, std::vector<int>{116}, std::vector<int>{116, 147}));
    EXPECT_EQ(textTape, tape(std::string(), pNPCTopics[135].pText));
    EXPECT_EQ(goldTape, tape(200));
}

GAME_TEST(MM6Transport, LastStableFinishesPriceFixing) {
    // Nothing set the bit Loretta Fleise's reward reads once all nine stables had agreed to fix their prices.
    startNewGameThroughInput(game);
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    pParty->_questBits.set(static_cast<QuestBit>(116)); // Loretta Fleise's price fixing topic in outd1 sets it.
    for (int bit = 148; bit <= 155; bit++)
        pParty->_questBits.set(static_cast<QuestBit>(bit)); // The other eight stables have agreed.
    auto bitsTape = tapes.custom([] { return priceFixingBits(); });
    test.startTaping();

    enterNewSorpigalStable(game);
    game.pressGuiButton("HouseDialogue_Option3");
    game.tick();

    EXPECT_EQ(bitsTape, tape(std::vector<int>{116, 148, 149, 150, 151, 152, 153, 154, 155},
                             std::vector<int>{116, 117, 147, 148, 149, 150, 151, 152, 153, 154, 155}));
}
