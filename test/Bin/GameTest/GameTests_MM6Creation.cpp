#include <algorithm>
#include <cstdint>
#include <exception>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>

#include "Testing/Game/GameTest.h"

#include "Engine/Engine.h"
#include "Engine/MM6/MM6CreationRules.h"
#include "Engine/Party.h"

GAME_TEST(MM6ZCreationHeadless, OriginalConditionPriorityFeedsHPAndSP) {
    ASSERT_EQ(engine->gameTitle(), GameTitle::MM6);

    struct Observation {
        int personalityBoth = 0;
        int personalityDisease = 0;
        int personalityNeutral = 0;
        int enduranceBoth = 0;
        int enduranceDisease = 0;
        int enduranceNeutral = 0;
        int manaBoth = 0;
        int manaDisease = 0;
        int manaNeutral = 0;
        int healthBoth = 0;
        int healthDisease = 0;
        int healthNeutral = 0;
        int expectedManaDisease = 0;
        int expectedHealthDisease = 0;
        std::string error;
    } result;

    game.runGameRoutine([&] {
        auto previous = std::make_unique<Party>(*pParty);
        try {
            Character &character = pParty->pCharacters[0];
            character.Zero();
            for (SpellBuff &buff : pParty->pPartyBuffs)
                buff.Reset();
            character.classType = static_cast<Class>(3);  // MM6 Cleric, Personality SP.
            character.uLevel = 10;
            const int birthYear = character.GetBaseAge() - 20;
            if (birthYear < 0 || birthYear > std::numeric_limits<std::uint16_t>::max())
                throw std::runtime_error("MM6 test cannot establish age 20");
            character.uBirthYear = birthYear;
            character._stats[ATTRIBUTE_PERSONALITY] = 20;
            character._stats[ATTRIBUTE_ENDURANCE] = 20;

            character.conditions.set(CONDITION_DRUNK, Time::fromDays(1));
            character.conditions.set(CONDITION_DISEASE_MEDIUM, Time::fromDays(1));
            result.personalityBoth = character.GetActualPersonality();
            result.enduranceBoth = character.GetActualEndurance();
            result.manaBoth = character.GetMaxMana();
            result.healthBoth = character.GetMaxHealth();

            character.conditions.reset(CONDITION_DRUNK);
            result.personalityDisease = character.GetActualPersonality();
            result.enduranceDisease = character.GetActualEndurance();
            result.manaDisease = character.GetMaxMana();
            result.healthDisease = character.GetMaxHealth();

            character.conditions.reset(CONDITION_DISEASE_MEDIUM);
            result.personalityNeutral = character.GetActualPersonality();
            result.enduranceNeutral = character.GetActualEndurance();
            result.manaNeutral = character.GetMaxMana();
            result.healthNeutral = character.GetMaxHealth();

            const mm6::CreationRules &rules = mm6::creationRules();
            result.expectedManaDisease = std::max(0, rules.spBase[1] +
                rules.spFactor[3] * (10 + character.GetParameterBonus(12)));
            result.expectedHealthDisease = std::max(1, rules.hpBase[1] +
                rules.hpFactor[3] * (10 + character.GetParameterBonus(6)));
        } catch (const std::exception &exception) {
            result.error = exception.what();
        } catch (...) {
            result.error = "unknown MM6 creation test exception";
        }
        *pParty = *previous;
    });

    ASSERT_TRUE(result.error.empty()) << result.error;
    EXPECT_EQ(result.personalityBoth, 12);  // Original disease factor 60%; drunk is lower priority.
    EXPECT_EQ(result.enduranceBoth, 6);     // Original disease factor 30%.
    EXPECT_EQ(result.personalityDisease, 12);
    EXPECT_EQ(result.enduranceDisease, 6);
    EXPECT_EQ(result.personalityNeutral, 20);
    EXPECT_EQ(result.enduranceNeutral, 20);
    EXPECT_EQ(result.manaBoth, result.expectedManaDisease);
    EXPECT_EQ(result.healthBoth, result.expectedHealthDisease);
    EXPECT_EQ(result.manaBoth, result.manaDisease);
    EXPECT_EQ(result.healthBoth, result.healthDisease);
    EXPECT_GT(result.manaNeutral, result.manaDisease);
    EXPECT_GT(result.healthNeutral, result.healthDisease);
}
