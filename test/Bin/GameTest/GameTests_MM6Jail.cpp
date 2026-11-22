#include <algorithm>
#include <string>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"

#include "MM6CampaignChain.h"

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

GAME_TEST(MM6Jail, ThroneRoomAtReputationMinus1000) {
    // A party at reputation -1000 walked into Castle Ironfist's throne room. MM6 sends it to jail for a year instead,
    // with no rest, and lets a party at -999 in.
    const MapId outd3 = pMapTable->GetMapInfo("outd3.odm");
    const std::string sentence = "For your numerous crimes and evil deeds, you have been sentenced to one year in prison. ";
    for (bool jailed : {true, false}) {
        SCOPED_TRACE(fmt::format("jailed={}", jailed));
        test.prepareForNextTest();
        startNewGameThroughInput(game);
        game.teleportTo(outd3, Vec3f(0, 0, 0), 0);
        pParty->mm6Reputation = jailed ? -1000 : -999;
        pParty->pCharacters[0].health = 1;
        pParty->pCharacters[1].timeToRecovery = Duration::fromRealtimeSeconds(60);
        auto houseTape = tapes.house();
        auto mapTape = tapes.map();
        auto yearTape = tapes.custom([] { return pParty->GetPlayingTime().toCivilTime().year; });
        auto reputationTape = tapes.custom([] { return pParty->mm6Reputation; });
        auto prisonTermsTape = tapes.custom([] { return pParty->uNumPrisonTerms; });
        auto awardTape = tapes.custom([] {
            return static_cast<int>(std::ranges::count_if(pParty->pCharacters, [](const Character &character) {
                return character._achievedAwardsBits[static_cast<AwardId>(83)]; // "Served %u Prison Terms".
            }));
        });
        auto foodTape = tapes.food();
        auto hpTape = charTapes.hp(0);
        auto recoveryTape = charTapes.isRecovering(1);
        auto textTape = tapes.allGUIWindowsText();
        test.startTaping();

        enterMM6CampaignChainCastle(game, outd3, 43);
        leaveMM6CampaignChainHouse(game);

        EXPECT_EQ(mapTape, tape(outd3));
        EXPECT_EQ(foodTape, tape(7));
        EXPECT_EQ(hpTape, tape(1));
        if (jailed) {
            EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(167), HOUSE_INVALID));
            EXPECT_EQ(yearTape, tape(1165, 1166));
            EXPECT_EQ(reputationTape, tape(-1000, 0));
            EXPECT_EQ(prisonTermsTape, tape(0, 1));
            EXPECT_EQ(awardTape, tape(0, 4));
            EXPECT_EQ(recoveryTape, tape(true, false));
            EXPECT_CONTAINS(textTape.flatten(), sentence);
        } else {
            EXPECT_EQ(houseTape, tape(HOUSE_INVALID, static_cast<HouseId>(154), HOUSE_INVALID));
            EXPECT_EQ(yearTape, tape(1165));
            EXPECT_EQ(reputationTape, tape(-999));
            EXPECT_EQ(prisonTermsTape, tape(0));
            EXPECT_EQ(awardTape, tape(0));
            EXPECT_EQ(recoveryTape, tape(true));
            EXPECT_FALSE(textTape.flatten().contains(sentence));
        }
    }
}
