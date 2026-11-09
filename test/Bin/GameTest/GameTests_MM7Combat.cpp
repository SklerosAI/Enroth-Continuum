#include <string_view>
#include <utility>

#include "Testing/Game/GameTest.h"

#include "Engine/AttackList.h"
#include "Engine/Engine.h"
#include "Engine/Localization.h"
#include "Engine/Party.h"
#include "Engine/Objects/Actor.h"
#include "Engine/Objects/Character.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/MonsterEnums.h"
#include "Engine/Objects/Monsters.h"
#include "Engine/Objects/SpriteObject.h"
#include "Engine/Spells/CastSpellInfo.h"
#include "Engine/Spells/Spells.h"
#include "Engine/Tables/NPCTable.h"
#include "GUI/UI/NPCTopics.h"
#include "GUI/UI/UIStatusBar.h"
#include "Library/Geometry/Rect.h"

MonsterProjectile ParseMissleAttackType(std::string_view missle_attack_str);
std::pair<int, int> MonsterPopup_Draw(unsigned int uActorID, Recti* pWindow);

GAME_TEST(MM7Combat, SpiritLashOutOfRangeDoesNotSpendMana) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    game.startNewGame();
    ASSERT_GT(pActors.size(), 10u);

    Character &caster = pParty->pCharacters[0];
    caster.setSkillValue(SKILL_SPIRIT, CombinedSkillValue(10, MASTERY_MASTER));
    caster.mana = 100;

    Actor &target = pActors[10];
    target.pos = pParty->pos + Vec3f(10000, 0, 0); // Beyond Spirit Lash's melee-only range.
    ASSERT_TRUE(target.CanBeDamaged());

    pushSpellOrRangedAttack(SPELL_SPIRIT_SPIRIT_LASH, 0, CombinedSkillValue::none(), 0, Pid(OBJECT_Actor, 10));
    game.tick();

    EXPECT_EQ(caster.mana, 100);
    EXPECT_EQ(engine->_statusBar->get(), "Spell failed");
}

GAME_TEST(MM7Combat, MonsterPopupBoundsInactiveSpellIds) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    game.startNewGame();
    ASSERT_FALSE(pActors.empty());

    Actor &actor = pActors[0];
    actor.aiState = AIState::Standing;
    actor.monsterInfo.spell1Id = static_cast<SpellId>(255);
    actor.monsterInfo.spell1UseChance = 0;
    actor.monsterInfo.spell2Id = static_cast<SpellId>(254);
    actor.monsterInfo.spell2UseChance = 0;
    engine->config->debug.FullMonsterID.setValue(true);

    auto [width, height] = MonsterPopup_Draw(0, nullptr);
    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

GAME_TEST(MM7Combat, MonsterPopupKeepsRegularSpellNames) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    game.startNewGame();
    ASSERT_FALSE(pActors.empty());

    Actor &actor = pActors[0];
    actor.monsterInfo.spell1Id = SPELL_FIRE_FIRE_BOLT;
    actor.monsterInfo.spell2Id = SPELL_AIR_SPARKS;
    engine->config->debug.FullMonsterID.setValue(true);

    EXPECT_FALSE(pSpellStats->pInfos[SPELL_FIRE_FIRE_BOLT].pShortName.empty());
    EXPECT_FALSE(pSpellStats->pInfos[SPELL_AIR_SPARKS].pShortName.empty());
    auto [width, height] = MonsterPopup_Draw(0, nullptr);
    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

GAME_TEST(MM6Popup, RegularMonsterSpellRenders) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);
    pActors.emplace_back();
    const unsigned int actorId = pActors.size() - 1;

    pActors[actorId].monsterInfo.spell1Id = SPELL_FIRE_FIRE_BOLT;
    pActors[actorId].monsterInfo.spell2Id = SPELL_NONE;
    engine->config->debug.FullMonsterID.setValue(true);

    auto [width, height] = MonsterPopup_Draw(actorId, nullptr);
    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
}

GAME_TEST(MM7NpcProfession, InvalidTitleFallsBackToName) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    game.startNewGame();

    NPCData npc;
    npc.name = "Synthetic NPC";
    npc.profession = static_cast<NpcProfession>(78); // First index beyond the shared profession table.
    EXPECT_EQ(NameAndTitle(&npc), npc.name);

    npc.profession = NPC_PROFESSION_NONE; // Quest NPCs can have no profession.
    EXPECT_EQ(NameAndTitle(&npc), npc.name);
    npc.profession = NPC_PROFESSION_SMITH;
    EXPECT_NE(NameAndTitle(&npc), npc.name);
}

GAME_TEST(MM7NpcProfession, InvalidProfessionHasNoHireOption) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    game.startNewGame();

    NPCData npc;
    npc.name = "Synthetic NPC";
    npc.canJoin = true;
    npc.profession = static_cast<NpcProfession>(78);
    EXPECT_TRUE(prepareScriptedNPCDialogueTopics(&npc).empty());

    npc.profession = NPC_PROFESSION_NONE;
    EXPECT_TRUE(prepareScriptedNPCDialogueTopics(&npc).empty());
    npc.profession = NPC_PROFESSION_SMITH;
    EXPECT_EQ(prepareScriptedNPCDialogueTopics(&npc).front(), DIALOGUE_13_hiring_related);
}

GAME_TEST(MM6NpcProfession, ValidTitleAndHireOptionRemain) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);

    NPCData npc;
    npc.name = "Synthetic NPC";
    npc.canJoin = true;
    npc.profession = NPC_PROFESSION_SMITH;
    EXPECT_NE(NameAndTitle(&npc), npc.name);
    EXPECT_EQ(prepareScriptedNPCDialogueTopics(&npc).front(), DIALOGUE_13_hiring_related);
}

static Pid appendCharacterDamageSprite(SpellId spell, Pid caster) {
    pSpriteObjects.emplace_back();
    SpriteObject &sprite = pSpriteObjects.back();
    sprite.uSpellID = spell;
    sprite.spell_caster_pid = caster;
    sprite.spell_level = 1;
    sprite.spell_skill = MASTERY_NOVICE;
    return Pid(OBJECT_Sprite, pSpriteObjects.size() - 1);
}

GAME_TEST(MM7CharacterSpriteDamage, RejectsSavedSpell255BeforePartyMutation) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    game.startNewGame();

    Character &target = pParty->pCharacters[0];
    target.health = 10000;
    target.conditions.set(CONDITION_SLEEP, Time::fromTicks(1));
    const Pid projectile = appendCharacterDamageSprite(static_cast<SpellId>(255), Pid(OBJECT_Sprite, 1000));

    DamageCharacterFromMonster(projectile, ABILITY_SPELL1, 0);
    EXPECT_EQ(target.health, 10000);
    EXPECT_TRUE(target.conditions.has(CONDITION_SLEEP));
}

GAME_TEST(MM7CharacterSpriteDamage, KeepsRegularSpellAndCharacterArrow) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    game.startNewGame();

    Character &target = pParty->pCharacters[0];
    target.health = 10000;
    for (auto [spell, caster] : {
             std::pair{SPELL_FIRE_FIRE_BOLT, Pid(OBJECT_Sprite, 1000)},
             std::pair{SPELL_BOW_ARROW, Pid(OBJECT_Character, 0)}}) {
        target.conditions.set(CONDITION_SLEEP, Time::fromTicks(1));
        const Pid projectile = appendCharacterDamageSprite(spell, caster);

        DamageCharacterFromMonster(projectile, ABILITY_SPELL1, 0);
        EXPECT_FALSE(target.conditions.has(CONDITION_SLEEP));
    }
}

GAME_TEST(MM6CharacterSpriteDamage, NumericCharacterArrowStillTraversesDamagePath) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);

    Character &target = pParty->pCharacters[0];
    target.health = 10000;
    target.conditions.set(CONDITION_SLEEP, Time::fromTicks(1));
    const Pid projectile = appendCharacterDamageSprite(SPELL_BOW_ARROW, Pid(OBJECT_Character, 0));

    DamageCharacterFromMonster(projectile, ABILITY_SPELL1, 0);
    EXPECT_FALSE(target.conditions.has(CONDITION_SLEEP));
}

GAME_TEST(MM7Combat, BreakWeaponAffectsEquippedSword) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    test.prepareForNextTest(10, RANDOM_ENGINE_SEQUENTIAL);
    game.startNewGame();

    Character &target = pParty->pCharacters[0];
    target.inventory.clear();
    InventoryEntry sword = target.inventory.equip(ITEM_SLOT_MAIN_HAND, Item(ITEM_BROADSWORD));
    ASSERT_FALSE(sword->IsBroken());

    for (int attempt = 0; attempt < 100 && !sword->IsBroken(); ++attempt)
        target.ReceiveSpecialAttackEffect(SPECIAL_ATTACK_BREAK_WEAPON, nullptr);

    EXPECT_TRUE(sword->IsBroken());
}

GAME_TEST(MM7Combat, BreakWeaponLeavesUnarmedTargetSafe) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    game.startNewGame();

    Character &target = pParty->pCharacters[0];
    target.inventory.clear();
    EXPECT_EQ(target.ReceiveSpecialAttackEffect(SPECIAL_ATTACK_BREAK_WEAPON, nullptr), 0);
}

GAME_TEST(MM7Combat, LichResistsMindAndBodyDamage) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    test.prepareForNextTest(10, RANDOM_ENGINE_SEQUENTIAL);
    game.startNewGame();

    Character &target = pParty->pCharacters[0];
    target.classType = CLASS_LICH;
    EXPECT_EQ(target.CalculateIncommingDamage(DAMAGE_MIND, 128), 0);
    EXPECT_EQ(target.CalculateIncommingDamage(DAMAGE_BODY, 128), 0);
    EXPECT_GT(target.CalculateIncommingDamage(DAMAGE_FIRE, 128), 0);

    target.classType = CLASS_SORCERER;
    EXPECT_GT(target.CalculateIncommingDamage(DAMAGE_MIND, 128), 0);
}

GAME_TEST(MM7Combat, DivineRestorationClearsTargetConditions) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    game.startNewGame();

    Character &target = pParty->pCharacters[0];
    Character &drinker = pParty->pCharacters[1];
    target.conditions.set(CONDITION_CURSED, Time::fromTicks(1));
    target.conditions.set(CONDITION_WEAK, Time::fromTicks(1));
    drinker.conditions.set(CONDITION_CURSED, Time::fromTicks(1));
    pParty->setActiveCharacterIndex(1);
    pParty->setHoldingItem(Item(ITEM_POTION_DIVINE_RESTORATION));

    drinker.useItem(0, true);

    EXPECT_FALSE(target.conditions.has(CONDITION_CURSED));
    EXPECT_FALSE(target.conditions.has(CONDITION_WEAK));
    EXPECT_TRUE(drinker.conditions.has(CONDITION_CURSED));
}

GAME_TEST(MM7Combat, FlamingArcherProjectileToken) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    EXPECT_EQ(ParseMissleAttackType("FireAr"), MONSTER_PROJECTILE_FLAMING_ARROW);
    EXPECT_EQ(ParseMissleAttackType("ARROWF"), MONSTER_PROJECTILE_FLAMING_ARROW);
}

GAME_TEST(MM7Combat, DivineInterventionRecoveryFallsWithLightSkill) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);

    auto recoveryAtLevel = [&](int level) {
        test.prepareForNextTest(10, RANDOM_ENGINE_SEQUENTIAL);
        game.startNewGame();

        Character &caster = pParty->pCharacters[0];
        caster.classType = CLASS_PRIEST_OF_SUN;
        caster.uLevel = 100;
        caster.setSkillValue(SKILL_LIGHT, CombinedSkillValue(level, MASTERY_GRANDMASTER));
        caster.bHaveSpell[SPELL_LIGHT_DIVINE_INTERVENTION] = true;
        caster.mana = caster.GetMaxMana();

        game.castSpell(0, SPELL_LIGHT_DIVINE_INTERVENTION);
        game.tick();
        EXPECT_EQ(caster.uNumDivineInterventionCastsThisDay, 1);
        return caster.timeToRecovery;
    };

    Duration lowSkillRecovery = recoveryAtLevel(1);
    Duration highSkillRecovery = recoveryAtLevel(10);
    EXPECT_GT(lowSkillRecovery, highSkillRecovery);
}

GAME_TEST(MM7Combat, DeathExplosionUsesSpecialDamage) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM7);
    game.startNewGame();
    ASSERT_FALSE(pActors.empty());

    pSpriteObjects.clear();
    attackList.clear();
    Actor &actor = pActors[0];
    actor.monsterInfo.specialAbilityDamageDiceRolls = 0;
    actor.monsterInfo.specialAbilityDamageDiceSides = 0;
    actor.monsterInfo.specialAbilityDamageDiceBonus = 37;
    actor.monsterInfo.field_3C_some_special_attack = std::to_underlying(DAMAGE_FIRE);

    EXPECT_EQ(actor._43B3E0_CalcDamage(ABILITY_SPECIAL), 37);
    EXPECT_EQ(actor.abilityDamageType(ABILITY_SPECIAL), DAMAGE_FIRE);

    Actor::Explode(0);
    ASSERT_EQ(pSpriteObjects.size(), 1u);
    EXPECT_EQ(pSpriteObjects[0].spellCasterAbility, ABILITY_SPECIAL);
    processSpellImpact(0, Pid());

    ASSERT_EQ(attackList.size(), 1u);
    EXPECT_EQ(attackList[0].attackSpecial, ABILITY_SPECIAL);
    attackList.clear();
}
