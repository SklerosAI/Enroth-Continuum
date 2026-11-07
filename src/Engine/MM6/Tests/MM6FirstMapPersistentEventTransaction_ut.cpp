#include <stdexcept>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6FirstMapPersistentEventTransaction.h"

namespace {

UNIT_TEST(MM6FirstMapPersistentEvents, RejectsForeignTitleAndMissingOwner) {
    PersistentVariables variables = {};
    variables.mapVars[0] = 7;
    variables.decorVars[124] = 9;
    EXPECT_THROW(mm6::ScopedFirstMapPersistentEvents(GameTitle::MM7, &variables), std::exception);
    EXPECT_THROW(mm6::ScopedFirstMapPersistentEvents(GameTitle::MM6, nullptr), std::exception);
    EXPECT_EQ(variables.mapVars[0], 7);
    EXPECT_EQ(variables.decorVars[124], 9);
}

UNIT_TEST(MM6FirstMapPersistentEvents, LateFaultRestoresMapAndDecorationArrays) {
    PersistentVariables variables = {};
    variables.mapVars[0] = 3;
    variables.mapVars[74] = 7;
    variables.decorVars[0] = 11;
    variables.decorVars[124] = 13;
    const PersistentVariables prior = variables;

    try {
        mm6::ScopedFirstMapPersistentEvents scope(GameTitle::MM6, &variables);
        // Outdoor delta reconstruction can replace both arrays; decoration
        // respawn and map-load events can then write them again.
        variables.mapVars.fill(41);
        variables.decorVars.fill(0);
        variables.decorVars[0] = 31;
        variables.mapVars[74] = 53;
        throw std::runtime_error("late map-load event failure");
    } catch (const std::runtime_error &) {
    }

    EXPECT_EQ(variables.mapVars, prior.mapVars);
    EXPECT_EQ(variables.decorVars, prior.decorVars);
}

UNIT_TEST(MM6FirstMapPersistentEvents, CommitRetainsReconstructedEvents) {
    PersistentVariables variables = {};
    variables.mapVars[0] = 3;
    variables.decorVars[0] = 11;
    {
        mm6::ScopedFirstMapPersistentEvents scope(GameTitle::MM6, &variables);
        variables.mapVars[0] = 37;
        variables.decorVars[0] = 43;
        scope.commit();
        scope.rollback();
    }
    EXPECT_EQ(variables.mapVars[0], 37);
    EXPECT_EQ(variables.decorVars[0], 43);
}

} // namespace
