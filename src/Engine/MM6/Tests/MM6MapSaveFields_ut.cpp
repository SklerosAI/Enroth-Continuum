#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6MapSaveFields.h"

namespace {

mm6::SavedMapDelta map(std::string name, std::string bytes) {
    return {std::move(name), {bytes.begin(), bytes.end()}};
}

void appendDecodedDeltas(const char *directory, std::string_view extension, mm6::MapSaveFields *fields) {
    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
        if (entry.path().extension() != extension)
            continue;
        std::ifstream stream(entry.path(), std::ios::binary);
        if (!stream)
            throw std::runtime_error("Cannot read ignored MM6 map fixture");
        std::string bytes(std::istreambuf_iterator<char>{stream}, {});
        fields->deltas.push_back(map(entry.path().filename().string(), std::move(bytes)));
    }
}

} // namespace

UNIT_TEST(MM6MapSaveFields, RoundTripAndRejectBeforeMutation) {
    mm6::MapSaveFields source{{map("outa1.ddm", "delta-a"), map("cd1.dlv", "delta-b")}};
    mm6::SaveContainer container{{mm6::encodeMapSection(source)}};
    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer(container);
    EXPECT_EQ(mm6::decodeMapSection(mm6::decodeSaveContainer(bytes)), source);

    mm6::MapSaveFields destination{{map("safe.dlv", "previous")}};
    bytes.back() ^= 1;
    EXPECT_THROW(mm6::loadMapFieldsBeforeMutation(bytes, &destination), std::exception);
    EXPECT_EQ(destination.deltas.size(), 1);
    EXPECT_EQ(destination.deltas.front().name, "safe.dlv");

    source.deltas.push_back(map("outa1.ddm", "duplicate"));
    EXPECT_THROW(mm6::encodeMapSection(source), std::exception);
    source.deltas.back().name = "../escape.dlv";
    EXPECT_THROW(mm6::encodeMapSection(source), std::exception);
    source.deltas.back().name = "other.dlv";
    source.deltas.back().bytes.clear();
    EXPECT_THROW(mm6::encodeMapSection(source), std::exception);
}

UNIT_TEST(MM6MapSaveFields, RetainedSixtySevenDeltas) {
    const char *indoor = std::getenv("ENROTH_MM6_INDOOR_DECODED_DIR");
    const char *outdoor = std::getenv("ENROTH_MM6_OUTDOOR_DECODED_DIR");
    if (!indoor || !outdoor)
        GTEST_SKIP() << "Set ignored decoded indoor and outdoor MM6 fixture directories";

    mm6::MapSaveFields source;
    appendDecodedDeltas(indoor, ".dlv", &source);
    appendDecodedDeltas(outdoor, ".ddm", &source);
    ASSERT_EQ(source.deltas.size(), 67);
    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer({{mm6::encodeMapSection(source)}});
    mm6::MapSaveFields restored;
    mm6::loadMapFieldsBeforeMutation(bytes, &restored);
    EXPECT_EQ(restored, source);
}
