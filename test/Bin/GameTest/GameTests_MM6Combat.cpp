#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/EngineGlobals.h"
#include "Engine/MM6/MM6ChestMapLoadRoute.h"
#include "Engine/MM6/MM6ScopedNewGameSave.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/CharacterEnumFunctions.h"
#include "Engine/Objects/Chest.h"
#include "Engine/Party.h"
#include "Engine/SaveLoad.h"
#include "Engine/Tables/MapTable.h"

#include "GUI/GUIWindow.h"

#include "Utility/String/Format.h"

#include "MM6MapEntranceForTest.h"

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

void enterMapAtShippedEntrance(EngineController &game, std::string_view fileName) {
    // Loads the map at the first shipped MoveToMap that names it, the way the map sweep does.
    const MapId map = pMapTable->GetMapInfo(fileName);
    const std::optional<MM6ShippedEntrance> entrance = findShippedMM6Entrance(fileName);
    ASSERT_TRUE(entrance);
    bool loaded = false;
    game.runGameRoutine([&] {
        for (Chest &chest : vChests)
            chest.position.reset(); // The first map's chest positions would make the target's chest load refuse.
        engine->_pendingTransition = MapDestination(map, entrance->placement);
        mm6::ScopedChestMapLoadOptIn chestOptIn;
        PrepareWorld(0);
        loaded = true;
    });
    while (!loaded)
        game.tick(); // The loading screen draws frames.
    ASSERT_EQ(engine->_currentLoadedMapId, map);
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

GAME_TEST(MM6CombatHeadless, DISABLED_CureWeaknessFromSpellbook) {
    // A new Body caster knows Cure Weakness, casting it from the spellbook used to refuse and end the run.
    // Run alone in a fresh MM6 headless process.
    auto mpTape = charTapes.mps();
    auto conditionTape = charTapes.conditions();
    auto recoveringTape = charTapes.areRecovering();
    enterFirstMapFromNewGame(game);

    auto caster = std::ranges::find_if(pParty->pCharacters, [](const Character &character) {
        return character.bHaveSpell[static_cast<SpellId>(67)];
    });
    ASSERT_NE(caster, pParty->pCharacters.end());
    const int casterIndex = static_cast<int>(caster - pParty->pCharacters.begin());
    const int targetIndex = (casterIndex + 1) % 4;
    pParty->pCharacters[targetIndex].SetCondition(CONDITION_WEAK, false);
    test.startTaping();

    game.pressAndReleaseKey(static_cast<PlatformKey>(std::to_underlying(PlatformKey::KEY_DIGIT_1) + casterIndex));
    game.tick();
    game.pressGuiButton("Game_CastSpell");
    game.tick();
    game.pressGuiButton(fmt::format("SpellBook_School{}", std::to_underlying(MAGIC_SCHOOL_BODY)));
    game.tick();
    game.pressGuiButton("SpellBook_Spell0"); // Selects the spell.
    game.tick();
    game.pressGuiButton("SpellBook_Spell0"); // Casts it, the book closes and the target picker opens next frame.
    game.tick(2);
    game.pressGuiButton(fmt::format("CastSpell_Character{}", targetIndex + 1));
    game.tick();

    EXPECT_LT(mpTape.back()[casterIndex], mpTape.front()[casterIndex]);
    EXPECT_EQ(conditionTape.front()[targetIndex], CONDITION_WEAK);
    EXPECT_EQ(conditionTape.back()[targetIndex], CONDITION_GOOD);
    EXPECT_FALSE(recoveringTape.front()[casterIndex]);
    EXPECT_TRUE(recoveringTape.back()[casterIndex]);
}

GAME_TEST(MM6CombatHeadless, DISABLED_ElectricTrapIsResistedByElectricity) {
    // An MM6 trap that dealt damage other than physical or fire used to refuse and end the run.
    // Run alone in a fresh MM6 headless process.
    auto hpTape = charTapes.hps();
    auto conditionTape = charTapes.conditions();
    auto trapTape = tapes.questBit(static_cast<QuestBit>(316));
    enterFirstMapFromNewGame(game);
    enterMapAtShippedEntrance(game, "t6.blv");
    game.teleportTo(engine->_currentLoadedMapId, Vec3f(3712, 1100, 1792), 270); // In front of a box that shocks the party.

    for (Character &character : pParty->pCharacters)
        character.health = 200; // A new party dies to the trap.
    pParty->pCharacters[0].sResAirBase = 10000; // The shared runtime keeps MM6 Electricity, Cold and Poison in Air, Water and Body.
    pParty->pCharacters[1].sResWaterBase = 10000;
    pParty->pCharacters[2].sResBodyBase = 10000;
    pParty->pCharacters[3].sResMagicBase = 10000;
    test.startTaping();

    game.pointMouseAtFace(2239); // The box's near side.
    game.pressAndReleaseButton(BUTTON_LEFT);
    game.tick();

    EXPECT_EQ(trapTape, tape(false, true));
    EXPECT_EQ(hpTape, tape({200, 200, 200, 200}, {196, 125, 125, 125})); // Only Electricity resistance halves the 75.
    EXPECT_EQ(conditionTape.frontBack(), tape({CONDITION_GOOD, CONDITION_GOOD, CONDITION_GOOD, CONDITION_GOOD},
                                              {CONDITION_GOOD, CONDITION_GOOD, CONDITION_GOOD, CONDITION_GOOD}));
}

GAME_TEST(MM6CombatHeadless, DISABLED_PoisonAndMagicMonstersHitParty) {
    // A monster hit with a Poison or Magic attack used to refuse and end the run.
    // Run alone in a fresh MM6 headless process.
    auto hpTape = tapes.totalHp();
    auto attackTape = tapes.custom([] {
        std::vector<DamageType> types;
        for (const Actor &actor : pActors)
            if (actor.aiState == AttackingMelee)
                types.push_back(actor.monsterInfo.attack1Type);
        return types;
    });
    enterFirstMapFromNewGame(game);
    test.startTaping();

    for (int monster : {74, 115}) {
        Actor *attacker = game.spawnMonster(pParty->pos + Vec3f(0, 150, 0), static_cast<MonsterId>(monster), SPAWN_STATIONARY);
        attacker->monsterInfo.specialAttackType = SPECIAL_ATTACK_NONE; // Their special attacks are still refused.
    }
    game.tick(25); // The monsters walk up and strike.

    auto attacks = attackTape.flatten();
    EXPECT_CONTAINS(attacks, DAMAGE_MAGIC);
    EXPECT_CONTAINS(attacks, DAMAGE_EARTH); // MM6 Poison.
    EXPECT_LT(hpTape.delta(), 0);
}
