#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Testing/Unit/MM6TestData.h"
#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6Archive.h"
#include "Engine/MM6/MM6MapSaveFields.h"

namespace {

mm6::SavedMapDelta map(std::string name, std::string bytes) {
    return {std::move(name), {bytes.begin(), bytes.end()}};
}

void appendShippedDeltas(const MM6TestData &data, std::string_view extension, mm6::MapSaveFields *fields) {
    for (const std::string &name : data.mapNames(extension))
        fields->deltas.push_back(map(name, std::string(decodeMM6ArchiveEntry(data.games().read(name)).str())));
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

UNIT_TEST(MM6MapSaveFields, ShippedSixtySevenDeltas) {
    const auto data = MM6TestData::open();
    if (!data)
        GTEST_SKIP() << mm6TestDataMissing;

    mm6::MapSaveFields source;
    appendShippedDeltas(*data, ".dlv", &source);
    appendShippedDeltas(*data, ".ddm", &source);
    ASSERT_EQ(source.deltas.size(), 67);
    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer({{mm6::encodeMapSection(source)}});
    mm6::MapSaveFields restored;
    mm6::loadMapFieldsBeforeMutation(bytes, &restored);
    EXPECT_EQ(restored, source);
}

UNIT_TEST(MM6MapSaveFields, ExactExpectedNameSetBeforeMutation) {
    mm6::MapSaveFields source;
    std::vector<std::string> expected;
    for (int i = 0; i < 67; ++i) {
        std::string name = "synthetic" + std::to_string(i) + ".dlv";
        expected.push_back(name);
        source.deltas.push_back(map(name, "delta"));
    }
    std::vector<std::uint8_t> bytes = mm6::encodeSaveContainer({{mm6::encodeMapSection(source)}});
    mm6::MapSaveFields destination{{map("safe.dlv", "previous")}};
    mm6::loadMapFieldsBeforeMutation(bytes, &destination, expected);
    EXPECT_EQ(destination, source);
    EXPECT_THROW(mm6::loadMapFieldsBeforeMutation(bytes, nullptr, expected), std::exception);

    destination = {{map("safe.dlv", "previous")}};
    expected.back() = "absent.dlv";
    EXPECT_THROW(mm6::loadMapFieldsBeforeMutation(bytes, &destination, expected), std::exception);
    EXPECT_EQ(destination.deltas.front().name, "safe.dlv");
    expected.back() = expected.front();
    EXPECT_THROW(mm6::validateExpectedMapNames(source, expected), std::exception);
    expected.pop_back();
    EXPECT_THROW(mm6::validateExpectedMapNames(source, expected), std::exception);
}
