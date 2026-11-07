#include <cstdint>
#include <string>

#include "Testing/Unit/UnitTest.h"

#include "Library/Image/Pcx.h"

#include "Utility/Exception.h"

namespace {

std::string encodedPcx(Sizei size) {
    Blob data = pcx::encode(RgbaImage::solid(Color(10, 20, 30), size));
    return std::string(data.str());
}

void setHeaderInt16(std::string *data, size_t offset, int16_t value) {
    const uint16_t bits = static_cast<uint16_t>(value);
    (*data)[offset] = static_cast<char>(bits & 0xff);
    (*data)[offset + 1] = static_cast<char>(bits >> 8);
}

} // namespace

UNIT_TEST(Pcx, DecodeSize) {
    // decodeSize() reads the same size from the header that decode() produces.
    for (Sizei size : {Sizei(1, 1), Sizei(3, 2), Sizei(17, 64)}) {
        Blob data = pcx::encode(RgbaImage::solid(Color(10, 20, 30), size));
        EXPECT_EQ(pcx::decodeSize(data), size);
        EXPECT_EQ(pcx::decode(data).size(), size);
    }
}

UNIT_TEST(Pcx, RejectsInvalidDimensionsBeforeAllocation) {
    std::string zeroHeight = encodedPcx({2, 1});
    setHeaderInt16(&zeroHeight, 6, 2); // ymin > ymax.
    EXPECT_THROW(pcx::decodeSize(Blob::fromString(zeroHeight)), Exception);
    EXPECT_THROW(pcx::decode(Blob::fromString(zeroHeight)), Exception);

    std::string zeroWidth = encodedPcx({2, 1});
    setHeaderInt16(&zeroWidth, 4, 2); // xmin > xmax.
    EXPECT_THROW(pcx::decodeSize(Blob::fromString(zeroWidth)), Exception);

    std::string huge = encodedPcx({2, 1});
    setHeaderInt16(&huge, 8, 4095); // 4096 by 4097 exceeds the decoded-pixel cap.
    setHeaderInt16(&huge, 10, 4096);
    setHeaderInt16(&huge, 66, 4096);
    EXPECT_THROW(pcx::decodeSize(Blob::fromString(huge)), Exception);

    std::string zeroPitch = encodedPcx({2, 1});
    setHeaderInt16(&zeroPitch, 66, 0);
    EXPECT_THROW(pcx::decodeSize(Blob::fromString(zeroPitch)), Exception);
}

UNIT_TEST(Pcx, RejectsTruncatedAndInvalidRuns) {
    std::string truncated = encodedPcx({2, 1});
    truncated.pop_back();
    EXPECT_THROW(pcx::decode(Blob::fromString(truncated)), Exception);

    std::string header = encodedPcx({2, 1}).substr(0, 128);
    EXPECT_THROW(pcx::decode(Blob::fromString(header + std::string("\xc0\x11", 2))), Exception);
    EXPECT_THROW(pcx::decode(Blob::fromString(header + std::string("\xc1", 1))), Exception);
    EXPECT_THROW(pcx::decode(Blob::fromString(header + std::string("\xcf\x11", 2))), Exception);
}

UNIT_TEST(Pcx, UncompressedRowsRequireAllBytes) {
    std::string header = encodedPcx({2, 1}).substr(0, 128);
    header[2] = 0; // No compression.
    std::string valid = header + std::string("\x0a\x14\x1e\x28\x32\x3c", 6);
    RgbaImage decoded = pcx::decode(Blob::fromString(valid));
    EXPECT_EQ(decoded.size(), Sizei(2, 1));
    EXPECT_EQ(decoded[0][0], Color(10, 30, 50));
    EXPECT_EQ(decoded[0][1], Color(20, 40, 60));
    valid.pop_back();
    EXPECT_THROW(pcx::decodeSize(Blob::fromString(valid)), Exception);
    EXPECT_THROW(pcx::decode(Blob::fromString(valid)), Exception);
}
