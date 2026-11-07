#include <algorithm>
#include <exception>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6PersistentEventPublication.h"

namespace {

mm6::DetachedPersistentEventBoundary stagedEvents() {
    mm6::DetachedPersistentEventBoundary staged;
    staged.mapBytes[0] = 17;
    staged.mapBytes[70] = 34;
    return staged;
}

std::vector<std::uint8_t> liveDelta(
    const mm6::DetachedPersistentEventBoundary &staged) {
    std::vector<std::uint8_t> delta(500);
    const auto source = mm6::capturePersistentEventBoundary(staged, GameTitle::MM6);
    std::copy(source.begin(), source.end(), delta.end() - 256);
    return delta;
}

} // namespace

UNIT_TEST(MM6PersistentEventPublication, FreshChildPublishesOnlyZeroGenericOwner) {
    const auto staged = stagedEvents();
    PersistentVariables destination{};
    destination.mapVars[0] = 3;
    destination.decorVars[124] = 8;
    mm6::publishFreshChildPersistentEventZeros(&destination, staged, GameTitle::MM6);
    EXPECT_EQ(destination.mapVars, (PersistentVariables{}).mapVars);
    EXPECT_EQ(destination.decorVars, (PersistentVariables{}).decorVars);

    destination.mapVars[0] = 3;
    auto invalid = staged;
    invalid.legacy.decorVars[124] = 1;
    EXPECT_THROW(mm6::publishFreshChildPersistentEventZeros(
        &destination, invalid, GameTitle::MM6), std::exception);
    EXPECT_EQ(destination.mapVars[0], 3);
    EXPECT_THROW(mm6::publishFreshChildPersistentEventZeros(
        &destination, staged, GameTitle::MM7), std::exception);
    EXPECT_EQ(destination.mapVars[0], 3);
    EXPECT_THROW(mm6::publishFreshChildPersistentEventZeros(
        nullptr, staged, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6PersistentEventPublication, PostMapRefusesChangedSidecarAndLegacyWrites) {
    const auto staged = stagedEvents();
    auto delta = liveDelta(staged);
    PersistentVariables legacy{};
    EXPECT_NO_THROW(mm6::requirePostMapPersistentEvents(
        staged, delta, legacy, GameTitle::MM6));

    const auto original = delta;
    delta[delta.size() - 256 + 70]++;
    EXPECT_THROW(mm6::requirePostMapPersistentEvents(
        staged, delta, legacy, GameTitle::MM6), std::exception);
    EXPECT_EQ(delta[delta.size() - 256 + 70], original[original.size() - 256 + 70] + 1);
    delta = original;
    delta[delta.size() - 256 + 199] = 1;
    EXPECT_THROW(mm6::requirePostMapPersistentEvents(
        staged, delta, legacy, GameTitle::MM6), std::exception);
    delta = original;
    legacy.mapVars[74] = 1;
    EXPECT_THROW(mm6::requirePostMapPersistentEvents(
        staged, delta, legacy, GameTitle::MM6), std::exception);
    legacy.mapVars[74] = 0;
    legacy.decorVars[124] = 1;
    EXPECT_THROW(mm6::requirePostMapPersistentEvents(
        staged, delta, legacy, GameTitle::MM6), std::exception);
    legacy.decorVars[124] = 0;
    EXPECT_THROW(mm6::requirePostMapPersistentEvents(
        staged, delta, legacy, GameTitle::MM7), std::exception);
    delta.resize(255);
    EXPECT_THROW(mm6::requirePostMapPersistentEvents(
        staged, delta, legacy, GameTitle::MM6), std::exception);
}
