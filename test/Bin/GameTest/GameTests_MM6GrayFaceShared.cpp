#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Data/AwardEnums.h"
#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Image.h"
#include "Engine/Graphics/Outdoor.h"
#include "Engine/Graphics/Sprites.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/NPC.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"
#include "Engine/Tables/NPCTable.h"
#include "Engine/Tables/TextureFrameTable.h"

#include "GUI/GUIButton.h"
#include "GUI/GUIWindow.h"
#include "GUI/UI/UIRest.h"

#include "Utility/Math/TrigLut.h"

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

static Vec3f ahead() {
    return Vec3f(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
}

static Vec3f side() {
    return Vec3f(-ahead().y, ahead().x, 0);
}

static void clickNpcOption(EngineController &game, std::string_view label) {
    // Left click in the middle of the conversation topic with this label.
    ASSERT_NE(pDialogueWindow, nullptr);
    std::optional<Pointi> center;
    for (GUIButton *button : pDialogueWindow->vButtons)
        if (button->id.starts_with("NpcDialogue_Option") && button->label == label)
            center = button->rect.center();
    ASSERT_TRUE(center) << "No conversation topic " << label;
    game.pressAndReleaseButton(BUTTON_LEFT, *center);
    game.tick();
}

static void waitInRestScreen(EngineController &game, std::string_view button) {
    game.pressGuiButton(button);
    game.tick();
    while (currentRestType != REST_NONE)
        game.tick(); // The wait passes.
}

static void waitFiveMinutes(EngineController &game) {
    game.pressAndReleaseButton(BUTTON_LEFT, {140, 320}); // The Wait 5 Minutes picture.
    game.tick();
    while (currentRestType != REST_NONE)
        game.tick(); // The wait passes.
}

GAME_TEST(MM6GrayFaceShared, ExplosionHitsParalyzedMonster) {
    // Area damage skipped paralyzed monsters in vanilla MM6. A Percival arrow's explosion hits a paralyzed goblin next
    // to its target, and a stoned goblin stays unhurt.
    startNewGameThroughInput(game);
    Character &archer = pParty->pCharacters[0];
    archer.inventory.equip(ITEM_SLOT_BOW, Item(static_cast<ItemId>(405))); // Percival.
    archer.setSkillValue(SKILL_BOW, CombinedSkillValue::novice());
    const Time tomorrow = pParty->GetPlayingTime() + Duration::fromDays(1);
    const Vec3f center = pParty->pos + ahead() * 1000;
    const int target = game.spawnMonster(center, static_cast<MonsterId>(76), SPAWN_DUMMY)->id;
    Actor *paralyzed = game.spawnMonster(center + side() * 200, static_cast<MonsterId>(76), SPAWN_DUMMY);
    paralyzed->buffs[ACTOR_BUFF_PARALYZED].Apply(tomorrow, MASTERY_MASTER, 0, 0, 0);
    Actor *stoned = game.spawnMonster(center - side() * 200, static_cast<MonsterId>(76), SPAWN_DUMMY);
    stoned->buffs[ACTOR_BUFF_STONED].Apply(tomorrow, MASTERY_MASTER, 0, 0, 0);
    paralyzed->hp = stoned->hp = 100; // A goblin would die to the explosion, and dying ends a paralysis.
    auto hpTape = actorTapes.hps({target, paralyzed->id, stoned->id});
    auto paralyzedTape = actorTapes.hasBuff(paralyzed->id, ACTOR_BUFF_PARALYZED);
    auto spritesTape = tapes.sprites();
    test.startTaping();

    game.pointMouseAtActor(target);
    for (int frame = 0; frame < 300 && hpTape.back()[0] == 13; frame++) { // Arrows can miss.
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }

    EXPECT_CONTAINS(spritesTape.flatten(), static_cast<SpriteId>(1051)); // The explosion.
    EXPECT_EQ(hpTape.front(), tape(13, 100, 100));
    EXPECT_LT(hpTape.back()[0], 13);
    EXPECT_LT(hpTape.back()[1], 100);
    EXPECT_EQ(hpTape.back()[2], 100);
    EXPECT_EQ(paralyzedTape, tape(true));
}

GAME_TEST(MM6GrayFaceShared, ScholarAddsFivePercentExperience) {
    // A Scholar hireling gave no experience bonus in vanilla MM6. With one, every character gets 5% more for a kill.
    for (bool scholar : {false, true}) {
        SCOPED_TRACE(fmt::format("scholar={}", scholar));
        test.prepareForNextTest();
        startNewGameThroughInput(game);
        getNPCData(pActors[0].npcId)->profession = NPC_PROFESSION_SCHOLAR; // New Sorpigal's street has no Scholar, Shelley becomes one.
        game.runGameRoutine([] { pParty->SetGold(1000, true); });
        auto experienceTape = charTapes.experiences();
        auto hirelingsTape = tapes.totalHirelings();
        test.startTaping();

        if (scholar) {
            game.teleportTo(engine->_currentLoadedMapId, pActors[0].pos - Vec3f(150, 0, 0), 0);
            game.pointMouseAtActor(0);
            game.pressAndReleaseButton(BUTTON_LEFT);
            game.tick();
            clickNpcOption(game, "Join");
            clickNpcOption(game, "Hire");
        }

        Actor *kingActor = game.spawnMonster(pParty->pos + ahead() * 150, static_cast<MonsterId>(78), SPAWN_DUMMY);
        kingActor->hp = 1; // Experience comes from the Goblin King's stats, a weak one dies sooner.
        const int king = kingActor->id;
        game.pointMouseAtActor(king);
        for (int frame = 0; frame < 100 && !pActors[king].IsNotAlive(); frame++) { // Blows can miss.
            game.pressAndReleaseKey(PlatformKey::KEY_A);
            game.tick();
        }
        ASSERT_TRUE(pActors[king].IsNotAlive());

        EXPECT_EQ(hirelingsTape, scholar ? tape(0, 1) : tape(0));
        for (int i = 0; i < 4; i++) // A quarter of the Goblin King's 200, and 5% on top with the Scholar.
            EXPECT_EQ(experienceTape.back()[i] - experienceTape.front()[i], scholar ? 52 : 50) << i;
    }
}

GAME_TEST(MM6GrayFaceShared, BlowSkipsDyingMonster) {
    // The party could attack a monster in its dying animation in vanilla MM6. A blow aimed at a dying goblin lands on
    // the live goblin next to it.
    startNewGameThroughInput(game);
    Actor *dying = game.spawnMonster(pParty->pos + ahead() * 150, static_cast<MonsterId>(76), SPAWN_DUMMY);
    dying->hp = 1;
    const int dyingId = dying->id;
    const int live = game.spawnMonster(pParty->pos + ahead() * 150 + side() * 150, static_cast<MonsterId>(76), SPAWN_DUMMY)->id;
    auto hpTape = actorTapes.hps({dyingId, live});
    auto dyingStateAndLiveHpTape = tapes.custom([=] { return std::pair(pActors[dyingId].aiState, pActors[live].hp); });
    auto experienceTape = tapes.totalExperience();
    test.startTaping();

    game.pointMouseAtActor(dyingId);
    for (int frame = 0; frame < 100 && pActors[dyingId].aiState != Dying; frame++) { // Blows can miss.
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }
    ASSERT_EQ(pActors[dyingId].aiState, Dying);
    while (pActors[dyingId].aiState == Dying) {
        game.pointMouseAtActor(dyingId);
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }

    EXPECT_EQ(hpTape.front(), tape(1, 13));
    EXPECT_EQ(hpTape.back()[0], 0);
    EXPECT_TRUE(dyingStateAndLiveHpTape.contains([](const auto &frame) { return frame.first == Dying && frame.second < 13; }));
    EXPECT_EQ(experienceTape.delta(), 56); // One goblin's experience.
}

GAME_TEST(MM6GrayFaceShared, HitWithoutRecoveryKeepsActiveCharacter) {
    // A hit that Endurance reduced to no recovery still passed the turn to the next character in vanilla MM6. A
    // character tough enough stays active through two hits, one who isn't hands over to the next.
    for (bool tough : {false, true}) {
        SCOPED_TRACE(fmt::format("tough={}", tough));
        test.prepareForNextTest();
        startNewGameThroughInput(game);
        for (Character &character : pParty->pCharacters) {
            character.health = 200; // The thug hits the others before the first one, and would knock them out.
            if (tough)
                character._stats[ATTRIBUTE_ENDURANCE] = 350; // Endurance 350 gives the +20 that cancels the 20 ticks of hit recovery.
        }
        auto activeTape = tapes.activeCharacterIndex();
        auto hpTape = charTapes.hp(0);
        auto recoveryTape = charTapes.custom(0, [](const Character &character) { return character.timeToRecovery; });
        test.startTaping();

        game.spawnMonster(pParty->pos + ahead() * 200, static_cast<MonsterId>(64), SPAWN_STATIONARY); // A thug, goblins run.
        for (int frame = 0; frame < 1000 && hpTape.size() < 3; frame++)
            game.tick(); // The thug strikes until it has hit the first character twice.
        ASSERT_EQ(hpTape.size(), 3);

        if (tough) {
            EXPECT_EQ(activeTape, tape(0));
            EXPECT_EQ(recoveryTape, tape(0_ticks));
        } else {
            EXPECT_EQ(activeTape.front(), 0);
            EXPECT_CONTAINS(activeTape, 1);
            EXPECT_GT(recoveryTape.max(), 0_ticks);
        }
    }
}

GAME_TEST(MM6GrayFaceShared, NewDayAtMonthStartAfterWaitUntilDawnAndEsc) {
    // Pressing Esc during "Wait until Dawn" skipped the New Day effects when the jump crossed into a new month in
    // vanilla MM6. Mid-month and at the month's end alike, the party that went without rest turns weak and eats.
    for (bool monthEnd : {false, true}) {
        SCOPED_TRACE(fmt::format("monthEnd={}", monthEnd));
        test.prepareForNextTest();
        startNewGameThroughInput(game);
        if (monthEnd) {
            pMapTable->pInfos[engine->_currentLoadedMapId].encounterChance = 0;
            pParty->SetFood(40); // A Rest and Heal a day for four weeks.
            for (int day = 0; day < 40 && pParty->uCurrentDayOfMonth < 27; day++) {
                game.pressAndReleaseKey(PlatformKey::KEY_R);
                game.tick();
                game.pressGuiButton("Rest_RestAndHeal");
                game.tick();
                while (current_screen_type == SCREEN_REST)
                    game.tick(); // The party sleeps eight hours.
                game.pressAndReleaseKey(PlatformKey::KEY_R);
                game.tick();
                waitInRestScreen(game, "Rest_WaitTillDawn");
                game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
                game.tick();
            }
            ASSERT_EQ(pParty->uCurrentDayOfMonth, 27);
        }
        auto weakTape = charTapes.custom([](const Character &character) { return character.conditions.has(CONDITION_WEAK); });
        auto foodTape = tapes.food();
        auto dateTape = tapes.custom([] { return std::pair(pParty->uCurrentMonth, pParty->uCurrentDayOfMonth); });
        auto clockTape = tapes.custom([] { return std::pair(pParty->uCurrentHour, pParty->uCurrentMinute); });
        test.startTaping();

        game.pressAndReleaseKey(PlatformKey::KEY_R);
        game.tick();
        game.pressGuiButton("Rest_WaitTillDawn");
        game.tick();
        game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
        game.tick();

        EXPECT_EQ(weakTape.frontBack(), tape({false, false, false, false}, {true, true, true, true}));
        EXPECT_EQ(foodTape.delta(), -1);
        EXPECT_EQ(clockTape.back(), std::pair(5, 0));
        EXPECT_EQ(dateTape.frontBack(), monthEnd ? tape(std::pair(0, 27), std::pair(1, 0)) : tape(std::pair(0, 0), std::pair(0, 1)));
    }
}

GAME_TEST(MM6GrayFaceShared, SunRisesAtFiveAm) {
    // Outdoors stayed at the last daytime light for the first minute after 5:00 AM in vanilla MM6. At 5:00 the sun is
    // on the horizon and the terrain at its darkest daytime level.
    startNewGameThroughInput(game);
    auto sunTape = tapes.custom([] {
        return std::tuple(pParty->uCurrentHour, pParty->uCurrentMinute, pOutdoor->vSunlight, pOutdoor->max_terrain_dimming_level);
    });
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    while (pParty->uCurrentHour != 4)
        waitInRestScreen(game, "Rest_Wait1Hour");
    while (pParty->uCurrentMinute < 55)
        waitFiveMinutes(game);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    while (pParty->uCurrentHour != 5 || pParty->uCurrentMinute != 1)
        game.tick(); // The night ends.

    auto atFive = sunTape.filter([](const auto &frame) { return std::get<0>(frame) == 5 && std::get<1>(frame) == 0; });
    ASSERT_FALSE(atFive.empty());
    for (const auto &[hour, minute, sun, dimming] : atFive) {
        EXPECT_EQ(sun, Vec3f(1, 0, 0));
        EXPECT_EQ(dimming, 20);
    }
}

GAME_TEST(MM6GrayFaceShared, WaitTicksRecovery) {
    // Waiting in the Rest dialog didn't tick the characters' recovery in vanilla MM6. After all four swing, a wait
    // brings the first one back while the Rest dialog is still open.
    startNewGameThroughInput(game);
    auto activeTape = tapes.activeCharacterIndex();
    auto screenTape = tapes.screen();
    test.startTaping();

    for (int i = 0; i < 4; i++) {
        game.pressAndReleaseKey(PlatformKey::KEY_A); // Swing at the air.
        game.tick();
    }
    game.pressAndReleaseKey(PlatformKey::KEY_R);
    game.tick();
    waitInRestScreen(game, "Rest_Wait1Hour");

    EXPECT_EQ(activeTape, tape(1, 2, 3, -1, 0));
    EXPECT_EQ(screenTape, tape(SCREEN_GAME, SCREEN_REST));
}

GAME_TEST(MM6GrayFaceShared, AwardsFollowActiveCharacter) {
    // The awards page kept the previous character's awards after switching characters in vanilla MM6.
    startNewGameThroughInput(game);
    pParty->pCharacters[0]._achievedAwardsBits.set(static_cast<AwardId>(1)); // Returned the Prince.
    pParty->pCharacters[1]._achievedAwardsBits.set(static_cast<AwardId>(7)); // Ended Winter.
    auto textTape = tapes.drawnTexts();
    auto activeTape = tapes.activeCharacterIndex();
    test.startTaping();

    game.pressAndReleaseKey(PlatformKey::KEY_I);
    game.tick(2); // The key queues the character screen for the next frame.
    game.pressAndReleaseKey(PlatformKey::KEY_A);
    game.tick();
    game.pressAndReleaseKey(PlatformKey::KEY_DIGIT_2);
    game.tick();

    auto awardLines = textTape.map([](const auto &frame) {
        std::vector<std::string> lines;
        for (const DrawnText &text : frame)
            if (text.text == "Returned the Prince" || text.text == "Ended Winter")
                lines.push_back(text.text);
        return lines;
    }).unique();
    EXPECT_EQ(activeTape, tape(0, 1));
    EXPECT_EQ(awardLines, tape(std::vector<std::string>(), std::vector<std::string>{"Returned the Prince"}, std::vector<std::string>{"Ended Winter"}));
}

GAME_TEST(MM6GrayFaceShared, AnimationFramesTurnWhenTheirTimeIsUp) {
    // The first frame of an MM6 sprite or texture animation lasted a tick too long and the last one a tick too short
    // in vanilla MM6. Each frame gives way to the next exactly when its time is up.
    for (int id = 0; id < static_cast<int>(pSpriteFrameTable->pSpriteSFrames.size()); id++) {
        const SpriteFrame &first = pSpriteFrameTable->pSpriteSFrames[id];
        if (first.spriteName.empty() || !(first.flags & SPRITE_FRAME_HAS_MORE) || !first.animationLength || !first.frameLength)
            continue;
        pSpriteFrameTable->InitializeSprite(id);
        if (!pSpriteFrameTable->pSpriteSFrames[id + 1].sprites[0])
            continue; // Lntnrev0's frames name Lntrev1 to Lntrev5, which sprites.lod doesn't have.
        SCOPED_TRACE(first.spriteName);
        EXPECT_EQ(pSpriteFrameTable->GetFrame(id, first.frameLength - 1_ticks), &pSpriteFrameTable->pSpriteSFrames[id]);
        EXPECT_EQ(pSpriteFrameTable->GetFrame(id, first.frameLength), &pSpriteFrameTable->pSpriteSFrames[id + 1]);
    }
    for (auto [firstName, secondName] : {std::pair("john01", "john02"), std::pair("paladn01", "paladn02")}) {
        SCOPED_TRACE(firstName);
        const int animation = pTextureFrameTable->animationId(firstName);
        ASSERT_GE(animation, 0);
        const Duration length = pTextureFrameTable->animationFrameLength(animation);
        EXPECT_EQ(pTextureFrameTable->animationFrame(animation, length - 1_ticks)->name(), firstName);
        EXPECT_EQ(pTextureFrameTable->animationFrame(animation, length)->name(), secondName);
    }
}
