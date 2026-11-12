#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Party.h"

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

GAME_TEST(MM6Bow, ArrowHitsMonster) {
    // MM6 bows never shot, the arrow was MM7's object 545 and MM6 has no such object.
    startNewGameThroughInput(game);
    Character &archer = pParty->pCharacters[0];
    archer.inventory.equip(ITEM_SLOT_BOW, Item(static_cast<ItemId>(42))); // Longbow.
    archer.setSkillValue(SKILL_BOW, CombinedSkillValue::novice());
    const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
    const int goblin = game.spawnMonster(pParty->pos + ahead * 1000, static_cast<MonsterId>(76), SPAWN_DUMMY)->id;
    auto hpTape = actorTapes.hp(goblin);
    auto spritesTape = tapes.sprites();
    test.startTaping();

    game.pointMouseAtActor(goblin);
    for (int frame = 0; frame < 100 && hpTape.back() == hpTape.front(); frame++) { // Arrows can miss.
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }

    EXPECT_CONTAINS(spritesTape.flatten(), static_cast<SpriteId>(500));
    EXPECT_EQ(hpTape.front(), 13);
    EXPECT_LT(hpTape.back(), 13);
}
