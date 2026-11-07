#include <memory>
#include <stdexcept>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Party.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"

namespace {

void completeEditableMM6Party() {
    for (Character &character : pParty->pCharacters) {
        for (int order = 4; order <= 12 && character.GetSkillIdxByOrder(3) == SKILL_INVALID; ++order) {
            const Skill skill = character.GetSkillIdxByOrder(order);
            if (skill != SKILL_INVALID && !character.pActiveSkills[skill])
                character.pActiveSkills[skill] = CombinedSkillValue::novice();
        }
        if (character.GetSkillIdxByOrder(3) == SKILL_INVALID)
            throw std::runtime_error("MM6 creation party lacks four skills");
    }
    for (int pass = 0; pass < 100 && CharacterCreation_GetUnspentAttributePointCount() > 0; ++pass)
        for (Character &character : pParty->pCharacters)
            for (Attribute stat : allStatAttributes())
                character.IncreaseAttribute(stat);
    if (CharacterCreation_GetUnspentAttributePointCount() != 0)
        throw std::runtime_error("MM6 creation party could not spend attribute points");
}

void enterFirstMapFromNewGame(EngineController &game) {
    // Default party through the creation screen, then the first map in the real game loop. The test-only new-game
    // callback seeds the map cache from the loaded archives and writes no save.
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    std::unique_ptr<mm6::ScopedChestMapLoadOptIn> chestOptIn;
    mm6::testing::setNewGameSaveOverride([] {
        auto firstMap = mm6::prepareNewGameMM6FromLoadedMaps();
        pMapDeltas.swap(firstMap.session.staged.worldAndVisuals.world.mapDeltas);
        mm6::chestTitleState() = firstMap.session.staged.worldAndVisuals.chestTitle;
    });
    game.runGameRoutine([&] { chestOptIn = std::make_unique<mm6::ScopedChestMapLoadOptIn>(); });
    game.pressGuiButton("MainMenu_NewGame");
    game.tick(2);
    ASSERT_NE(pGUIWindow_CurrentMenu, nullptr);
    ASSERT_EQ(pGUIWindow_CurrentMenu->eWindowType, WINDOW_PARTY_CREATION);
    game.runGameRoutine(completeEditableMM6Party);
    game.pressGuiButton("PartyCreation_OK");
    game.skipLoadingScreen();
    game.tick();
    game.runGameRoutine([&] { chestOptIn.reset(); });
    mm6::testing::setNewGameSaveOverride({});
    ASSERT_EQ(engine->_currentLoadedMapId, pMapTable->GetMapInfo("oute3.odm"));
    ASSERT_EQ(current_screen_type, SCREEN_GAME);
}

} // namespace

GAME_TEST(MM6CombatHeadless, DISABLED_FirstMapMonsterSpellsHitParty) {
    // Every spell a first-map monster casts used to refuse and end the run.
    // Run alone in a fresh MM6 headless process.
    auto hpTape = tapes.totalHp();
    auto conditionTape = charTapes.conditions();
    auto spritesTape = tapes.sprites();
    enterFirstMapFromNewGame(game);
    test.startTaping();

    // Goblin Shaman, Journeyman Mage and Mage cast Fire Bolt, Cold Beam and Lightning Bolt. The Apprentice Mage's
    // Flame Arrow isn't here, the apprentice is a wimp and runs instead of casting.
    for (int monster : {77, 137, 138}) {
        Actor *caster = game.spawnMonster(pParty->pos + Vec3f(0, 1500, 0), static_cast<MonsterId>(monster), SPAWN_STATIONARY);
        caster->monsterInfo.spell1UseChance = 100;
    }
    game.tick(60); // The casters turn to the party and cast a few rounds.

    auto sprites = spritesTape.flatten();
    for (int object : {1030, 2060, 3010})
        EXPECT_CONTAINS(sprites, static_cast<SpriteId>(object));
    EXPECT_LT(hpTape.delta(), 0);
    EXPECT_EQ(conditionTape.frontBack(), tape({CONDITION_GOOD, CONDITION_GOOD, CONDITION_GOOD, CONDITION_GOOD},
                                              {CONDITION_GOOD, CONDITION_GOOD, CONDITION_GOOD, CONDITION_GOOD}));
}

GAME_TEST(MM6CombatHeadless, DISABLED_GoblinKingBreaksItem) {
    // The Goblin King's item-breaking hit used to refuse and end the run.
    // Run alone in a fresh MM6 headless process.
    auto hpTape = tapes.totalHp();
    auto brokenTape = tapes.custom([] {
        int broken = 0;
        for (const Character &character : pParty->pCharacters)
            for (InventoryConstEntry entry : character.inventory.entries())
                broken += entry->IsBroken();
        return broken;
    });
    enterFirstMapFromNewGame(game);
    test.startTaping();

    Actor *king = game.spawnMonster(pParty->pos + Vec3f(0, 150, 0), static_cast<MonsterId>(78), SPAWN_STATIONARY);
    king->monsterInfo.spell1UseChance = 0;
    king->monsterInfo.specialAttackLevel = 10; // Level 10 times 10 makes every hit try to break an item.
    game.tick(100); // The king walks up and strikes.

    EXPECT_LT(hpTape.delta(), 0);
    EXPECT_EQ(brokenTape.front(), 0);
    EXPECT_GT(brokenTape.back(), 0);
}

