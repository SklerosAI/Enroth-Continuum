#include <string_view>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/Party.h"
#include "Engine/Objects/Character.h"
#include "Engine/Objects/ItemEnums.h"
#include "Engine/Objects/MonsterEnums.h"
#include "Engine/Objects/Monsters.h"

MonsterProjectile ParseMissleAttackType(std::string_view missle_attack_str);

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
