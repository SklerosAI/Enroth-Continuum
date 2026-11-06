#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6VoiceSound.h"

UNIT_TEST(MM6VoiceSound, UsesFaceBandAndSoundKind) {
    for (int face = 0; face < mm6::kPortraitFaceCount; ++face) {
        auto first = mm6::faceAnimationSoundId(GameTitle::MM6, face, 1, 0, 1);
        auto last = mm6::faceAnimationSoundId(GameTitle::MM6, face, 44, 0, 2);
        auto secondVariant = mm6::faceAnimationSoundId(GameTitle::MM6, face, 44, 1, 2);
        ASSERT_TRUE(first.has_value());
        ASSERT_TRUE(last.has_value());
        ASSERT_TRUE(secondVariant.has_value());
        EXPECT_EQ(*first, 5002 + 100 * face);
        EXPECT_EQ(*last, 5088 + 100 * face);
        EXPECT_EQ(*secondVariant, *last + 1);
    }
}

UNIT_TEST(MM6VoiceSound, RejectsForeignTitleAndOutOfRangeInputs) {
    EXPECT_FALSE(mm6::faceAnimationSoundId(GameTitle::MM7, 0, 1, 0, 1));
    EXPECT_FALSE(mm6::faceAnimationSoundId(GameTitle::MM6, -1, 1, 0, 1));
    EXPECT_FALSE(mm6::faceAnimationSoundId(GameTitle::MM6, 12, 1, 0, 1));
    EXPECT_FALSE(mm6::faceAnimationSoundId(GameTitle::MM6, 0, 0, 0, 1));
    EXPECT_FALSE(mm6::faceAnimationSoundId(GameTitle::MM6, 0, 45, 0, 1));
    EXPECT_FALSE(mm6::faceAnimationSoundId(GameTitle::MM6, 0, 1, -1, 1));
    EXPECT_FALSE(mm6::faceAnimationSoundId(GameTitle::MM6, 0, 1, 1, 1));
    EXPECT_FALSE(mm6::faceAnimationSoundId(GameTitle::MM6, 0, 1, 0, 0));
    EXPECT_FALSE(mm6::faceAnimationSoundId(GameTitle::MM6, 0, 1, 0, 3));
}
