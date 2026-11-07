#include <stdexcept>
#include <string>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6GameplayRngSaveFields.h"
#include "Engine/MM6/MM6RawSaveFields.h"
#include "Library/Random/MersenneTwisterRandomEngine.h"
#include "Library/Random/SequentialRandomEngine.h"

UNIT_TEST(MM6GameplayRngSaveFields, MersenneTwisterContinuationRoundTrips) {
    MersenneTwisterRandomEngine original;
    original.seed(173);
    for (int index = 0; index < 11; ++index)
        (void) original.random(1000);
    GameplayRandomState state;
    state.mtState = original.stateText();
    mm6::SaveContainer container{{mm6::encodeGameplayRngSection(state, GameTitle::MM6)}, 2};
    const auto decoded = mm6::decodeGameplayRngSection(
        mm6::decodeSaveContainer(mm6::encodeSaveContainer(container)), GameTitle::MM6);
    EXPECT_EQ(decoded, state);
    MersenneTwisterRandomEngine resumed;
    resumed.restoreStateText(decoded.mtState);
    for (int index = 0; index < 12; ++index)
        EXPECT_EQ(resumed.random(1000), original.random(1000));
    const int next = resumed.peek(1000);
    EXPECT_THROW(resumed.restoreStateText("not a state"), std::exception);
    EXPECT_EQ(resumed.peek(1000), next);
}

UNIT_TEST(MM6GameplayRngSaveFields, SequentialContinuationAndMalformedRefusal) {
    SequentialRandomEngine original;
    original.seed(22);
    (void) original.random(100);
    GameplayRandomState state;
    state.type = RANDOM_ENGINE_SEQUENTIAL;
    state.sequentialState = original.state();
    mm6::SaveContainer container{{mm6::encodeGameplayRngSection(state, GameTitle::MM6)}, 2};
    const auto decoded = mm6::decodeGameplayRngSection(container, GameTitle::MM6);
    EXPECT_EQ(decoded, state);
    SequentialRandomEngine resumed;
    resumed.restoreState(decoded.sequentialState);
    EXPECT_EQ(resumed.random(100), original.random(100));
    container.sections[0].payload.push_back(0);
    EXPECT_THROW((void) mm6::decodeGameplayRngSection(container, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::decodeGameplayRngSection(container, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::encodeGameplayRngSection(state, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::decodeRawSave(
        mm6::encodeSaveContainer({{mm6::encodeGameplayRngSection(state, GameTitle::MM6)}, 2}),
        GameTitle::MM6), std::exception);
}
