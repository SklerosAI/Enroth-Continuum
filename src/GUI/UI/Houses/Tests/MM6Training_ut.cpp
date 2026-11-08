#include "Testing/Unit/UnitTest.h"

#include "GUI/UI/Houses/MM6Training.h"

#include "Engine/Objects/Character.h"

UNIT_TEST(MM6Training, MaxLevelsAreHardcodedPerHall) {
    // MM6 caps each training hall by house id, and house 82 has no practical cap.
    EXPECT_EQ(mm6::trainingMaxLevel(79), 15);
    EXPECT_EQ(mm6::trainingMaxLevel(82), 65535);
    EXPECT_EQ(mm6::trainingMaxLevel(88), 50);
    EXPECT_ANY_THROW(mm6::trainingMaxLevel(78));
    EXPECT_ANY_THROW(mm6::trainingMaxLevel(89));
}

UNIT_TEST(MM6Training, ExperienceRequirement) {
    // Training past level n needs 1000 times the n-th triangular number of experience.
    EXPECT_EQ(mm6::trainingExperience(1), 1000);
    EXPECT_EQ(mm6::trainingExperience(2), 3000);
    EXPECT_EQ(mm6::trainingExperience(10), 55000);
}

UNIT_TEST(MM6Training, PriceUsesPromotionTierAndMerchantFloor) {
    // The price is promotion tier times level times the hall's value, lowered by merchant down to a third.
    Character character;
    character.uLevel = 3;
    character.classType = static_cast<Class>(0);
    EXPECT_EQ(mm6::trainingPrice(character, 10, 0), 30);
    character.classType = static_cast<Class>(5);
    EXPECT_EQ(mm6::trainingPrice(character, 10, 0), 90);
    EXPECT_EQ(mm6::trainingPrice(character, 10, 10), 81);
    EXPECT_EQ(mm6::trainingPrice(character, 10, 90), 30);
    EXPECT_EQ(mm6::trainingPrice(character, 10, 150), 30);
    EXPECT_EQ(mm6::trainingPrice(character, 2.5f, 0), 22);
}
