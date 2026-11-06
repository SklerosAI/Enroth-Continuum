#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Snapshots/MM6IndoorSnapshot.h"

#include "Utility/Exception.h"
#include "Utility/Memory/Blob.h"

namespace {

void putU32(std::string *data, size_t offset, uint32_t value) {
    for (int i = 0; i < 4; ++i)
        (*data)[offset + i] = static_cast<char>((value >> (8 * i)) & 0xFF);
}

Blob minimalBase() {
    // MM6 header plus ten zero-count sections; all variable arrays are empty.
    std::string data(136 + 10 * 4, '\0');
    putU32(&data, 136 + 4 * 4, 200); // Door slots follow vertices, faces, extras and rooms.
    return Blob::fromString(std::move(data));
}

Blob minimalDelta() {
    std::string data(8 + 875 + 3 * 4 + 200 * 80 + 200 + 56, '\0');
    return Blob::fromString(std::move(data));
}

Blob readFile(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw Exception("MM6 indoor fixture file could not be opened");
    return Blob::fromString(std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()));
}

void expectEqualBytes(const Blob &expected, const Blob &actual) {
    ASSERT_EQ(expected.size(), actual.size());
    EXPECT_EQ(std::memcmp(expected.data(), actual.data(), expected.size()), 0);
}

} // namespace

UNIT_TEST(MM6IndoorSnapshot, SyntheticRoundTripAndOpaqueMutation) {
    Blob baseBytes = minimalBase();
    MM6IndoorSnapshot base = deserializeMM6IndoorBase(baseBytes);
    EXPECT_EQ(base.section(MM6IndoorSectionKind::DoorSlots).count, 200);
    expectEqualBytes(baseBytes, serializeMM6Indoor(base));

    Blob deltaBytes = minimalDelta();
    MM6IndoorSnapshot delta = deserializeMM6IndoorDelta(deltaBytes, base);
    expectEqualBytes(deltaBytes, serializeMM6Indoor(delta));

    std::string changedTail(56, '\0');
    changedTail[0] = static_cast<char>(0xA5);
    delta.replaceSection(MM6IndoorSectionKind::Tail, Blob::fromString(std::move(changedTail)));
    Blob changed = serializeMM6Indoor(delta);
    ASSERT_EQ(deltaBytes.size(), changed.size());
    EXPECT_EQ(std::memcmp(deltaBytes.data(), changed.data(), changed.size() - 56), 0);
    EXPECT_NE(std::memcmp(static_cast<const char *>(deltaBytes.data()) + changed.size() - 56,
                          static_cast<const char *>(changed.data()) + changed.size() - 56, 56), 0);
}

UNIT_TEST(MM6IndoorSnapshot, RejectsTruncatedAndInconsistentData) {
    Blob baseBytes = minimalBase();
    MM6IndoorSnapshot base = deserializeMM6IndoorBase(baseBytes);
    EXPECT_THROW(deserializeMM6IndoorBase(baseBytes.subBlob(0, baseBytes.size() - 1)), Exception);

    Blob deltaBytes = minimalDelta();
    EXPECT_THROW(deserializeMM6IndoorDelta(deltaBytes.subBlob(0, deltaBytes.size() - 1), base), Exception);
    std::string wrongCount(static_cast<const char *>(deltaBytes.data()), deltaBytes.size());
    putU32(&wrongCount, 8 + 875, 0xFFFFFFFF);
    EXPECT_THROW(deserializeMM6IndoorDelta(Blob::fromString(std::move(wrongCount)), base), Exception);

    std::string negativeDoorCount(static_cast<const char *>(deltaBytes.data()), deltaBytes.size());
    const size_t doorRecordOffset = 8 + 875 + 3 * 4;
    negativeDoorCount[doorRecordOffset + 68] = static_cast<char>(0xFF);
    negativeDoorCount[doorRecordOffset + 69] = static_cast<char>(0xFF);
    EXPECT_THROW(deserializeMM6IndoorDelta(Blob::fromString(std::move(negativeDoorCount)), base), Exception);
}

UNIT_TEST(MM6IndoorSnapshot, RetainedFiftyTwoPairs) {
    const char *directory = std::getenv("ENROTH_MM6_INDOOR_DECODED_DIR");
    if (!directory)
        GTEST_SKIP() << "Set ENROTH_MM6_INDOOR_DECODED_DIR to the ignored decoded fixture directory";

    size_t pairs = 0;
    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
        if (entry.path().extension() != ".blv")
            continue;
        Blob baseBytes = readFile(entry.path());
        std::filesystem::path deltaPath = entry.path();
        deltaPath.replace_extension(".dlv");
        Blob deltaBytes = readFile(deltaPath);
        MM6IndoorSnapshot base = deserializeMM6IndoorBase(baseBytes);
        MM6IndoorSnapshot delta = deserializeMM6IndoorDelta(deltaBytes, base);
        expectEqualBytes(baseBytes, serializeMM6Indoor(base));
        expectEqualBytes(deltaBytes, serializeMM6Indoor(delta));

        for (MM6IndoorSectionKind kind : {MM6IndoorSectionKind::Visibility,
                                          MM6IndoorSectionKind::PersistentBytes,
                                          MM6IndoorSectionKind::Tail}) {
            size_t sectionOffset = 0;
            for (const MM6IndoorSection &part : delta.sections) {
                if (part.kind == kind)
                    break;
                sectionOffset += part.bytes.size();
            }
            Blob original = Blob::copy(delta.section(kind).bytes);
            std::string modified(static_cast<const char *>(original.data()), original.size());
            modified[0] ^= static_cast<char>(0xA5);
            delta.replaceSection(kind, Blob::fromString(std::move(modified)));

            std::string expected(static_cast<const char *>(deltaBytes.data()), deltaBytes.size());
            expected[sectionOffset] ^= static_cast<char>(0xA5);
            Blob encoded = serializeMM6Indoor(delta);
            ASSERT_EQ(encoded.size(), expected.size());
            EXPECT_EQ(std::memcmp(encoded.data(), expected.data(), expected.size()), 0);
            MM6IndoorSnapshot reparsed = deserializeMM6IndoorDelta(encoded, base);
            expectEqualBytes(delta.section(kind).bytes, reparsed.section(kind).bytes);
            delta.replaceSection(kind, original);
        }
        ++pairs;
    }
    EXPECT_EQ(pairs, 52);
}
