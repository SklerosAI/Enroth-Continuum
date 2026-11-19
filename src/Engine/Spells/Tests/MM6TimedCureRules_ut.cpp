#include <array>
#include <stdexcept>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Spells/MM6TimedCureRules.h"

#include "Utility/String/Format.h"

static constexpr std::array<int, 6> kIds = {40, 49, 57, 60, 64, 67};
static constexpr std::array<Condition, 6> kConditions = {CONDITION_PETRIFIED, CONDITION_CURSED, CONDITION_FEAR, CONDITION_PARALYZED, CONDITION_INSANE, CONDITION_WEAK};

static int windowSeconds(int spellId, int rank, mm6::TimedCureWindow window) {
    const bool tripled = window == mm6::TIMED_CURE_WINDOW_ORIGINAL && spellId >= 57;
    const std::array<int, 3> windows = {180, tripled ? 10800 : 3600, tripled ? 259200 : 86400};
    return windows[rank - 1];
}

UNIT_TEST(MM6TimedCureRules, SixConditionsAndStrictRankWindows) {
    // Stone to Flesh and Remove Curse take 3 minutes, 1 hour or 1 day per skill point, the other four 3 hours and 3
    // days at Expert and Master unless the spell descriptions' windows are asked for.
    const Time now = Time::fromDays(100);
    for (mm6::TimedCureWindow window : {mm6::TIMED_CURE_WINDOW_ORIGINAL, mm6::TIMED_CURE_WINDOW_DESCRIBED}) {
        for (size_t i = 0; i < kIds.size(); ++i) {
            const SpellId spell = static_cast<SpellId>(kIds[i]);
            EXPECT_TRUE(mm6::isTimedCureSpell(spell));
            for (int rank = 1; rank <= 3; ++rank) {
                SCOPED_TRACE(fmt::format("window={} spell={} rank={}", std::to_underlying(window), kIds[i], rank));
                constexpr int skill = 2;
                const Duration limit = Duration::fromSeconds(windowSeconds(kIds[i], rank, window) * skill);
                CharacterConditions conditions;
                conditions.set(kConditions[i], now - limit);
                conditions.set(CONDITION_SLEEP, now - Duration::fromSeconds(1));
                EXPECT_FALSE(mm6::applyTimedCure(spell, static_cast<Mastery>(rank), skill, now, conditions, window));
                EXPECT_TRUE(conditions.has(kConditions[i]));
                conditions.set(kConditions[i], now - limit + Duration::fromTicks(1));
                EXPECT_TRUE(mm6::applyTimedCure(spell, static_cast<Mastery>(rank), skill, now, conditions, window));
                EXPECT_FALSE(conditions.has(kConditions[i]));
                EXPECT_TRUE(conditions.has(CONDITION_SLEEP));
                EXPECT_FALSE(mm6::applyTimedCure(spell, static_cast<Mastery>(rank), skill, now, conditions, window));
            }
        }
    }
}

UNIT_TEST(MM6TimedCureRules, RejectsUnsupportedInputsBeforeMutation) {
    const Time now = Time::fromDays(10);
    const mm6::TimedCureWindow window = mm6::TIMED_CURE_WINDOW_DESCRIBED;
    CharacterConditions conditions;
    conditions.set(CONDITION_WEAK, now - Duration::fromSeconds(1));
    EXPECT_FALSE(mm6::isTimedCureSpell(SPELL_BODY_FIRST_AID));
    EXPECT_THROW(mm6::applyTimedCure(SPELL_BODY_FIRST_AID, MASTERY_NOVICE, 1, now, conditions, window), std::runtime_error);
    EXPECT_THROW(mm6::applyTimedCure(static_cast<SpellId>(67), MASTERY_GRANDMASTER, 1, now, conditions, window), std::runtime_error);
    EXPECT_THROW(mm6::applyTimedCure(static_cast<SpellId>(67), MASTERY_NOVICE, 0, now, conditions, window), std::runtime_error);
    EXPECT_THROW(mm6::applyTimedCure(static_cast<SpellId>(67), MASTERY_NOVICE, 64, now, conditions, window), std::runtime_error);
    EXPECT_TRUE(conditions.has(CONDITION_WEAK));
    conditions.set(CONDITION_WEAK, now + Duration::fromSeconds(1));
    EXPECT_THROW(mm6::applyTimedCure(static_cast<SpellId>(67), MASTERY_NOVICE, 1, now, conditions, window), std::runtime_error);
    EXPECT_TRUE(conditions.has(CONDITION_WEAK));
}
