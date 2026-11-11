#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Indoor.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/Resources/EngineFileSystem.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"

static constexpr QuestBit reactorDestroyed = static_cast<QuestBit>(180);
static constexpr QuestBit demonQueenKilled = static_cast<QuestBit>(202);
static constexpr QuestBit escapedWithRitual = static_cast<QuestBit>(237);
static constexpr ItemId ritualOfTheVoid = static_cast<ItemId>(544);
static constexpr int reactorActor = 0;
static constexpr int demonQueenActor = 1;

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

static void enterHiveArmed(EngineController &game) {
    // The first character carries a blaster and the skill to fire it. The party goes through the hive entrance in
    // outa1, and every hive monster but the reactor and the Demon Queen is gone, so a new party lives to reach them.
    Character &shooter = pParty->pCharacters[0];
    shooter.inventory.equip(ITEM_SLOT_MAIN_HAND, Item(static_cast<ItemId>(64)));
    shooter.setSkillValue(SKILL_BLASTER, CombinedSkillValue::novice());
    const MapId hive = pMapTable->GetMapInfo("hive.blv");
    game.runGameRoutine([=] { startMapTransition(MapDestination(hive, PartyPlacement(Vec3f(435, 3707, 1), 512, 0, 0))); });
    game.tick();
    game.skipLoadingScreen();
    game.tick();
    ASSERT_EQ(engine->_currentLoadedMapId, hive);
    ASSERT_EQ(pActors[reactorActor].monsterInfo.id, static_cast<MonsterId>(173));
    ASSERT_EQ(pActors[demonQueenActor].monsterInfo.id, static_cast<MonsterId>(172));
    for (Actor &actor : pActors)
        if (actor.id != reactorActor && actor.id != demonQueenActor)
            actor.aiState = Removed;
}

static void blast(EngineController &game, int actorId, Vec3f position, int viewYaw) {
    // The boss is down to its last hit point, a new party can't wear down 1300. The first character fires the blaster
    // at it until it falls.
    pActors[actorId].hp = 1;
    game.teleportTo(engine->_currentLoadedMapId, position, viewYaw);
    game.pointMouseAtActor(actorId);
    for (int shot = 0; shot < 100 && pActors[actorId].aiState != Dying && pActors[actorId].aiState != Dead; shot++) {
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }
    ASSERT_TRUE(pActors[actorId].aiState == Dying || pActors[actorId].aiState == Dead);
}

static void blastDemonQueen(EngineController &game) {
    blast(game, demonQueenActor, Vec3f(2800, 26880, -2255), 0);
}

static void blastReactor(EngineController &game) {
    blast(game, reactorActor, Vec3f(5216, 22464, -2255), 180);
}

static void waitForRecovery(EngineController &game) {
    // The obelisks only answer a character who has recovered from the fight.
    for (int frame = 0; frame < 100 && !pParty->hasActiveCharacter(); frame++)
        game.tick();
}

static void touchExitObelisk(EngineController &game) {
    // In front of the hive:60 obelisk, facing it.
    waitForRecovery(game);
    game.teleportTo(engine->_currentLoadedMapId, Vec3f(4956, 27270, -2261), 0);
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
}

static void waitOutEndingScreen(EngineController &game) {
    // The ending screen ignores keys for its first 5 seconds.
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick(50);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
}

GAME_TEST(MM6Finale, BlasterHurtsReactor) {
    // MM6 blasters dealt physical damage, which the hive reactor is immune to, so the game couldn't be won.
    startNewGameThroughInput(game);
    enterHiveArmed(game);
    auto hpTape = actorTapes.hp(reactorActor);
    test.startTaping();

    game.teleportTo(engine->_currentLoadedMapId, Vec3f(5216, 22464, -2255), 180); // In sight of the reactor.
    game.pointMouseAtActor(reactorActor);
    for (int shot = 0; shot < 10 && hpTape.back() == 1300; shot++) {
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }

    EXPECT_EQ(hpTape.front(), 1300);
    EXPECT_LT(hpTape.back(), 1300);
}

GAME_TEST(MM6Finale, WinsWithRitual) {
    // The MM6 hive's exit obelisk refused the winning ending as an unavailable passage. With the Ritual of the Void it
    // shows the win screen and the farewell box, then the party goes on in New Sorpigal.
    startNewGameThroughInput(game);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId hive = pMapTable->GetMapInfo("hive.blv");
    pParty->pCharacters[1].inventory.add(Item(ritualOfTheVoid));
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto queenTape = tapes.questBit(demonQueenKilled);
    auto reactorTape = tapes.questBit(reactorDestroyed);
    auto escapedTape = tapes.questBit(escapedWithRitual);
    auto ritualTape = tapes.hasItem(ritualOfTheVoid);
    auto actorCountTape = tapes.custom([=] { return engine->_currentLoadedMapId == hive ? static_cast<int>(pActors.size()) : 0; });
    auto doorTape = tapes.custom([=] {
        std::vector<DoorState> result;
        if (engine->_currentLoadedMapId == hive)
            for (int doorId : {28, 30, 51, 52, 53})
                for (const BLVDoor &door : pIndoor->doors)
                    if (door.doorId == doorId)
                        result.push_back(door.state);
        return result;
    });
    auto positionTape = tapes.custom([] { return pParty->pos.xy(); });
    auto certificateTape = tapes.custom([] { return ufs->exists("MM6_Win.Pcx"); });
    auto messageBoxesTape = tapes.messageBoxes();
    test.startTaping();

    enterHiveArmed(game);
    blastDemonQueen(game);
    blastReactor(game);
    touchExitObelisk(game);
    game.tick(); // The ending opens on the next message pass.
    ASSERT_EQ(screenTape.back(), SCREEN_GAMEOVER_WINDOW);
    waitOutEndingScreen(game);
    ASSERT_EQ(messageBoxesTape.flatten().size(), 1);
    game.pressAndReleaseKey(PlatformKey::KEY_ESCAPE);
    game.tick();
    game.skipLoadingScreen();
    game.tick();
    ufs->remove("MM6_Win.Pcx");

    EXPECT_EQ(mapTape, tape(oute3, hive, oute3));
    EXPECT_CONTAINS(screenTape, SCREEN_GAMEOVER_WINDOW);
    EXPECT_EQ(screenTape.back(), SCREEN_GAME);
    EXPECT_EQ(queenTape, tape(false, true));
    EXPECT_EQ(reactorTape, tape(false, true));
    EXPECT_EQ(escapedTape, tape(false, true));
    EXPECT_EQ(ritualTape, tape(true, false));
    EXPECT_EQ(actorCountTape, tape(0, 117, 133, 0)); // The reactor summons 16 hive guards.
    EXPECT_CONTAINS(doorTape, (std::vector{DOOR_OPEN, DOOR_OPEN, DOOR_CLOSED, DOOR_CLOSED, DOOR_OPEN}));
    EXPECT_CONTAINS(doorTape, (std::vector{DOOR_OPEN, DOOR_CLOSING, DOOR_OPENING, DOOR_OPENING, DOOR_CLOSING}));
    EXPECT_CONTAINS(positionTape, Vec2f(3328, 25920)); // The reactor puts the party at the hive exit.
    EXPECT_EQ(positionTape.back(), positionTape.front()); // New Sorpigal's party start, where a new game starts too.
    EXPECT_EQ(certificateTape, tape(false, true));
    auto messageBoxes = messageBoxesTape.flatten();
    ASSERT_EQ(messageBoxes.size(), 1);
    EXPECT_TRUE(messageBoxes.front().starts_with("Congratulations Adventurer."));
}

GAME_TEST(MM6Finale, ReactorWithoutRitualLoses) {
    // Blowing up the MM6 hive reactor without the Ritual of the Void loses the game at once and quits to the main
    // menu.
    startNewGameThroughInput(game);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId hive = pMapTable->GetMapInfo("hive.blv");
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto reactorTape = tapes.questBit(reactorDestroyed);
    auto deathsTape = tapes.deaths();
    auto menuTape = tapes.custom([] { return GetCurrentMenuID(); });
    auto certificateTape = tapes.custom([] { return ufs->exists("MM6_Win.Pcx"); });
    auto messageBoxesTape = tapes.messageBoxes();
    test.startTaping();

    enterHiveArmed(game);
    blastReactor(game);
    game.tick(); // The ending opens on the next message pass.
    ASSERT_EQ(screenTape.back(), SCREEN_GAMEOVER_WINDOW);
    waitOutEndingScreen(game);

    EXPECT_EQ(mapTape, tape(oute3, hive));
    EXPECT_CONTAINS(screenTape, SCREEN_GAMEOVER_WINDOW);
    EXPECT_EQ(reactorTape, tape(false));
    EXPECT_EQ(deathsTape, tape(0));
    EXPECT_EQ(menuTape.back(), MENU_MAIN);
    EXPECT_EQ(certificateTape, tape(false));
    EXPECT_TRUE(messageBoxesTape.flatten().empty());
}

GAME_TEST(MM6Finale, ObeliskWithoutRitualLoses) {
    // Leaving the MM6 hive through the obelisk after the Demon Queen falls, without the Ritual of the Void, loses the
    // game.
    startNewGameThroughInput(game);
    auto screenTape = tapes.screen();
    auto queenTape = tapes.questBit(demonQueenKilled);
    auto escapedTape = tapes.questBit(escapedWithRitual);
    auto menuTape = tapes.custom([] { return GetCurrentMenuID(); });
    auto certificateTape = tapes.custom([] { return ufs->exists("MM6_Win.Pcx"); });
    test.startTaping();

    enterHiveArmed(game);
    blastDemonQueen(game);
    touchExitObelisk(game);
    game.tick(); // The ending opens on the next message pass.
    ASSERT_EQ(screenTape.back(), SCREEN_GAMEOVER_WINDOW);
    waitOutEndingScreen(game);

    EXPECT_EQ(queenTape, tape(false, true));
    EXPECT_EQ(escapedTape, tape(false));
    EXPECT_EQ(menuTape.back(), MENU_MAIN);
    EXPECT_EQ(certificateTape, tape(false));
}

GAME_TEST(MM6Finale, DeathInHiveLoses) {
    // A party that died in the MM6 hive after the reactor blew threw instead of losing the game.
    startNewGameThroughInput(game);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId hive = pMapTable->GetMapInfo("hive.blv");
    pParty->pCharacters[1].inventory.add(Item(ritualOfTheVoid));
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto reactorTape = tapes.questBit(reactorDestroyed);
    auto deathsTape = tapes.deaths();
    auto menuTape = tapes.custom([] { return GetCurrentMenuID(); });
    test.startTaping();

    enterHiveArmed(game);
    blastReactor(game);
    for (Vec3f offset : {Vec3f(0, 200, 0), Vec3f(0, -200, 0), Vec3f(200, 0, 0), Vec3f(-200, 0, 0)}) {
        Actor *killer = game.spawnMonster(pParty->pos + offset, static_cast<MonsterId>(108), SPAWN_STATIONARY);
        killer->monsterInfo.spell1UseChance = 0; // Blows only, its Finger of Death would kill from afar.
    }
    for (int frame = 0; frame < 200 && current_screen_type != SCREEN_GAMEOVER_WINDOW; frame++)
        game.tick(); // The four monsters around the party strike everyone down.
    ASSERT_EQ(screenTape.back(), SCREEN_GAMEOVER_WINDOW);
    waitOutEndingScreen(game);

    EXPECT_EQ(mapTape, tape(oute3, hive));
    EXPECT_EQ(reactorTape, tape(false, true));
    EXPECT_EQ(deathsTape, tape(0)); // No death penalty, the game is over.
    EXPECT_EQ(menuTape.back(), MENU_MAIN);
}

GAME_TEST(MM6Finale, LeavingHiveLoses) {
    // Leaving the MM6 hive by its entrance after the reactor blew loses the game instead of reaching outa1.
    startNewGameThroughInput(game);
    const MapId oute3 = pMapTable->GetMapInfo("oute3.odm");
    const MapId hive = pMapTable->GetMapInfo("hive.blv");
    pParty->pCharacters[1].inventory.add(Item(ritualOfTheVoid));
    auto mapTape = tapes.map();
    auto screenTape = tapes.screen();
    auto reactorTape = tapes.questBit(reactorDestroyed);
    auto menuTape = tapes.custom([] { return GetCurrentMenuID(); });
    test.startTaping();

    enterHiveArmed(game);
    blastReactor(game);
    waitForRecovery(game);
    game.teleportTo(hive, Vec3f(448, 3638, -5), -90); // In front of the hive:48 obelisk by the entrance, facing it.
    game.pressAndReleaseKey(PlatformKey::KEY_SPACE);
    game.tick();
    game.pressGuiButton("Transition_Yes");
    game.tick();
    ASSERT_EQ(screenTape.back(), SCREEN_GAMEOVER_WINDOW);
    waitOutEndingScreen(game);

    EXPECT_EQ(mapTape, tape(oute3, hive));
    EXPECT_EQ(reactorTape, tape(false, true));
    EXPECT_EQ(menuTape.back(), MENU_MAIN);
}


