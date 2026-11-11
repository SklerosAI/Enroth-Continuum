#include <string>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6HouseExit.h"
#include "Engine/MM6/MM6RoomMovie.h"

#include "Utility/Memory/Blob.h"

UNIT_TEST(MM6HouseExit, ColumnsReadLikeAtoi) {
    // The original reads the exit columns with atoi, so a castle's map cell "2D 154" is 2 and "Need Key" is 0.
    EXPECT_EQ(mm6::houseExitField("Need Key"), 0);
    EXPECT_EQ(mm6::houseExitField("Dungeon"), 0);
    EXPECT_EQ(mm6::houseExitField("2D 154"), 2);
    EXPECT_EQ(mm6::houseExitField("167"), 167);
    EXPECT_EQ(mm6::houseExitField("-6"), -6);
    EXPECT_EQ(mm6::houseExitField(""), 0);
}

UNIT_TEST(MM6RoomMovie, LengthFromSmackerHeader) {
    // One pass of a room movie lasts its frame count times its frame time.
    auto header = [](int frames, int frameRate) {
        std::string result = "SMK2";
        for (int value : {460, 344, frames, frameRate})
            result.append(reinterpret_cast<const char *>(&value), sizeof(value));
        return Blob::fromString(result);
    };
    EXPECT_EQ(mm6::roomMovieLength(header(10, 100)), 1000);
    EXPECT_EQ(mm6::roomMovieLength(header(51, -6666)), 3400);
    EXPECT_EQ(mm6::roomMovieLength(header(5, 0)), 500);
    EXPECT_EQ(mm6::roomMovieLength(Blob::fromString("BIKi")), 0);
}
