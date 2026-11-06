#include <array>
#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6ActualStatCondition.h"

UNIT_TEST(MM6ActualStatCondition, UsesOriginalPriorityInsteadOfCompoundingConditions) {
    std::array<bool, 17> active{};
    auto has = [&](Condition condition) { return active[static_cast<unsigned>(condition)]; };

    EXPECT_EQ(mm6::actualStatCondition(GameTitle::MM6, has), CONDITION_GOOD);
    active[static_cast<unsigned>(CONDITION_DRUNK)] = true;
    active[static_cast<unsigned>(CONDITION_DISEASE_MEDIUM)] = true;
    EXPECT_EQ(mm6::actualStatCondition(GameTitle::MM6, has), CONDITION_DISEASE_MEDIUM);

    // The original Intellect/Personality condition row gives medium disease
    // 60% and drunkenness 25%. Selecting one yields 12 from a base of 20;
    // compounding them would yield 3, changing HP/SP stat-bonus thresholds.
    constexpr int baseStat = 20;
    constexpr int selectedPercent = 60;
    EXPECT_EQ(baseStat * selectedPercent / 100, 12);
    EXPECT_NE(baseStat * selectedPercent * 25 / 10000, 12);

    active[static_cast<unsigned>(CONDITION_ERADICATED)] = true;
    EXPECT_EQ(mm6::actualStatCondition(GameTitle::MM6, has), CONDITION_ERADICATED);
    EXPECT_THROW((void) mm6::actualStatCondition(GameTitle::MM7, has), std::runtime_error);
}

UNIT_TEST(MM6ActualStatCondition, CoversEveryOriginalConditionAndNeutral) {
    std::array<bool, 17> active{};
    auto has = [&](Condition condition) { return active[static_cast<unsigned>(condition)]; };
    for (Condition condition : mm6::kActualStatConditionPriority) {
        active[static_cast<unsigned>(condition)] = true;
        EXPECT_EQ(mm6::actualStatCondition(GameTitle::MM6, has), condition);
        active[static_cast<unsigned>(condition)] = false;
    }
    EXPECT_EQ(mm6::actualStatCondition(GameTitle::MM6, has), CONDITION_GOOD);
}
