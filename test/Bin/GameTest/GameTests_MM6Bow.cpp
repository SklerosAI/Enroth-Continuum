#include <algorithm>
#include <cstdlib>
#include <string>
#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Application/GameConfig.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/Graphics/Renderer/NullRenderer.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Party.h"
#include "Engine/Tables/MapTable.h"

#include "Library/Image/Png.h"

#include "Utility/Exception.h"
#include "Utility/Math/TrigLut.h"
#include "Utility/Streams/FileOutputStream.h"
#include "Utility/System/Path.h"

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

GAME_TEST(MM6Bow, ArrowIsDrawn) {
    // MM6 arrows in flight were drawn as azure sparkles instead of their sprite.
    startNewGameThroughInput(game);
    Character &archer = pParty->pCharacters[0];
    archer.inventory.equip(ITEM_SLOT_BOW, Item(static_cast<ItemId>(42))); // Longbow.
    archer.setSkillValue(SKILL_BOW, CombinedSkillValue::novice());
    const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
    const int goblin = game.spawnMonster(pParty->pos + ahead * 1000, static_cast<MonsterId>(76), SPAWN_DUMMY)->id;
    auto spritesTape = tapes.sprites();
    auto drawnTape = tapes.drawnSprites();
    test.startTaping();

    game.pointMouseAtActor(goblin);
    game.pressAndReleaseKey(PlatformKey::KEY_A);
    game.tick();

    EXPECT_CONTAINS(spritesTape.flatten(), static_cast<SpriteId>(500));
    EXPECT_CONTAINS(drawnTape.flatten(), static_cast<SpriteId>(500));
}

GAME_TEST(MM6Bow, PercivalArrowExplodes) {
    // MM6 Percival arrows never exploded, they hit like plain arrows.
    for (ItemId bow : {static_cast<ItemId>(42), static_cast<ItemId>(405)}) { // Longbow, Percival.
        SCOPED_TRACE(fmt::format("bow={}", std::to_underlying(bow)));
        test.prepareForNextTest();
        startNewGameThroughInput(game);
        Character &archer = pParty->pCharacters[0];
        archer.inventory.equip(ITEM_SLOT_BOW, Item(bow));
        archer.setSkillValue(SKILL_BOW, CombinedSkillValue::novice());
        const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
        const int target = game.spawnMonster(pParty->pos + ahead * 1000, static_cast<MonsterId>(76), SPAWN_DUMMY)->id;
        const Vec3f side(-ahead.y, ahead.x, 0);
        const int bystander = game.spawnMonster(pParty->pos + ahead * 1000 + side * 200, static_cast<MonsterId>(76), SPAWN_DUMMY)->id;
        auto targetHpTape = actorTapes.hp(target);
        auto bystanderHpTape = actorTapes.hp(bystander);
        auto spritesTape = tapes.sprites();
        test.startTaping();

        game.pointMouseAtActor(target);
        for (int frame = 0; frame < 300 && targetHpTape.back() == targetHpTape.front(); frame++) { // Arrows can miss.
            game.pressAndReleaseKey(PlatformKey::KEY_A);
            game.tick();
        }

        EXPECT_CONTAINS(spritesTape.flatten(), static_cast<SpriteId>(500));
        EXPECT_LT(targetHpTape.back(), 13);
        if (bow == static_cast<ItemId>(405)) {
            EXPECT_CONTAINS(spritesTape.flatten(), static_cast<SpriteId>(1051));
            EXPECT_LT(bystanderHpTape.back(), 13);
        } else {
            EXPECT_FALSE(spritesTape.flatten().contains(static_cast<SpriteId>(1051)));
            EXPECT_EQ(bystanderHpTape, tape(13));
        }
    }
}

GAME_TEST(MM6Missile, MonsterMissilesAreDrawn) {
    // MM6 monster arrows and fire bolts in flight were drawn as sparkles instead of their sprites.
    startNewGameThroughInput(game);
    const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
    const Vec3f side(-ahead.y, ahead.x, 0);
    game.spawnMonster(pParty->pos + ahead * 1000, static_cast<MonsterId>(1), SPAWN_DUMMY); // Archer.
    game.spawnMonster(pParty->pos + ahead * 1000 + side * 200, static_cast<MonsterId>(43), SPAWN_DUMMY); // Druid.
    auto spritesTape = tapes.sprites();
    auto drawnTape = tapes.drawnSprites();
    test.startTaping();

    for (int frame = 0; frame < 300 && !spritesTape.flatten().containsAll(500, 520); frame++) // The monsters notice the party and shoot.
        game.tick();

    EXPECT_CONTAINS(spritesTape.flatten(), static_cast<SpriteId>(500));
    EXPECT_CONTAINS(spritesTape.flatten(), static_cast<SpriteId>(520));
    EXPECT_CONTAINS(drawnTape.flatten(), static_cast<SpriteId>(500));
    EXPECT_CONTAINS(drawnTape.flatten(), static_cast<SpriteId>(520));
}

GAME_TEST(MM6Loot, DevilWarriorDropsPlans) {
    // The Devil Warrior took the Devil Plans with it, MM6 devils leave no corpse to loot.
    startNewGameThroughInput(game);
    const ItemId plans = static_cast<ItemId>(506);
    const int warrior = 0;
    game.teleportTo(pMapTable->GetMapInfo("zdwj02.blv"), Vec3f(-128, -300, 1), 90);
    ASSERT_EQ(pActors[warrior].carriedItemId, plans);
    pActors[warrior].hp = 1;
    engine->config->debug.NoDamage.setValue(true); // The other devils of the outpost attack the party.
    auto stateTape = actorTapes.aiState(warrior);
    auto floorTape = tapes.mapItemCount(plans);
    auto plansTape = tapes.hasItem(plans);
    test.startTaping();

    for (int frame = 0; frame < 300 && !pActors[warrior].IsNotAlive(); frame++) { // Blows can miss.
        try {
            game.pointMouseAtActor(warrior);
        } catch (const Exception &) {
            // The warrior comes for the party and can be out of view for a frame, A still attacks then.
        }
        game.pressAndReleaseKey(PlatformKey::KEY_A);
        game.tick();
    }
    for (int frame = 0; frame < 100 && pActors[warrior].aiState != Removed; frame++) // The warrior falls and vanishes.
        game.tick();
    auto sprite = std::ranges::find(pSpriteObjects, plans, [](const SpriteObject &sprite) { return sprite.containing_item.itemId; });
    ASSERT_NE(sprite, pSpriteObjects.end());
    game.teleportTo(engine->_currentLoadedMapId, sprite->vPosition - Vec3f(0, 150, 0), 90, -20); // Looking down at the plans.
    game.pointMouseAtSprite(sprite - pSpriteObjects.begin());
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();

    EXPECT_CONTAINS(stateTape, Dying);
    EXPECT_EQ(stateTape.back(), Removed);
    EXPECT_EQ(floorTape, tape(0, 1, 0));
    EXPECT_EQ(plansTape, tape(false, true));
}

GAME_TEST(MM6OffscreenFrame, DISABLED_MissileCapture) {
    // Hidden capture of a monster arrow and a monster fire bolt in flight, for checking how MM6 missiles look.
    ASSERT_FALSE(window->isVisible());
    ASSERT_EQ(dynamic_cast<NullRenderer *>(render), nullptr);
    const char *capturePath = std::getenv("MM6_OFFSCREEN_CAPTURE_PATH");
    ASSERT_NE(capturePath, nullptr);
    const std::string_view destination(capturePath);
    ASSERT_TRUE(destination.starts_with("/tmp/") || destination.starts_with("/private/tmp/"));
    ASSERT_TRUE(Path(capturePath).isNormalized());
    ASSERT_EQ(Path(capturePath).extension(), ".png");

    startNewGameThroughInput(game);
    const Vec3f ahead(TrigLUT.cos(pParty->_viewYaw), TrigLUT.sin(pParty->_viewYaw), 0);
    const Vec3f side(-ahead.y, ahead.x, 0);
    game.spawnMonster(pParty->pos + ahead * 1000 - side * 150, static_cast<MonsterId>(1), SPAWN_DUMMY); // Archer.
    game.spawnMonster(pParty->pos + ahead * 1000 + side * 150, static_cast<MonsterId>(43), SPAWN_DUMMY); // Druid.
    auto inFlight = [] (int spriteId) {
        return std::ranges::any_of(pSpriteObjects, [spriteId] (const SpriteObject &object) {
            return object.uObjectDescID != 0 && object.spriteId == static_cast<SpriteId>(spriteId);
        });
    };
    // The arrow is captured to the given path, the fire bolt next to it with a "-bolt" suffix.
    for (int spriteId : {500, 520}) {
        for (int frame = 0; frame < 300 && !inFlight(spriteId); frame++)
            game.tick();
        ASSERT_TRUE(inFlight(spriteId));

        RgbaImage frame;
        game.runGameRoutine([&frame] {
            render->ClearTarget(Color(0, 0, 0));
            engine->drawWorld();
            engine->drawHUD();
            render->flushAndScale();
            engine->drawOverlay();
            frame = render->MakeFullScreenshot();
        });
        ASSERT_EQ(frame.size(), Sizei(640, 480));
        std::string path(capturePath);
        if (spriteId == 520)
            path.insert(path.size() - 4, "-bolt");
        FileOutputStream output{Path(path)};
        output.write(png::encode(frame));
        output.close();
    }
}
