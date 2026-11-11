#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"

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
