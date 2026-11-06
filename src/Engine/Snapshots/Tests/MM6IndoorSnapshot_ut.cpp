#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>

#include "Testing/Unit/UnitTest.h"

#include "Engine/Snapshots/MM6IndoorSnapshot.h"
#include "Engine/Snapshots/CompositeSnapshots.h"

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

UNIT_TEST(MM6IndoorSnapshot, ProjectsMinimalBase) {
    MM6IndoorSnapshot base = deserializeMM6IndoorBase(minimalBase());
    IndoorLocation_MM7 projected = projectMM6IndoorBase(base);
    EXPECT_TRUE(projected.vertices.empty());
    EXPECT_TRUE(projected.faces.empty());
    EXPECT_EQ(projected.doorCount, 200);
}

UNIT_TEST(MM6IndoorSnapshot, EncodesKnownFieldsAndRejectsUnsupportedChanges) {
    MM6IndoorSnapshot base = deserializeMM6IndoorBase(minimalBase());
    MM6IndoorRuntimeDelta delta = decodeMM6IndoorRuntimeDelta(minimalDelta(), base);
    delta.respawnCount = 3;
    delta.lastRespawnDay = 42;
    delta.visibleOutlines[0] = static_cast<char>(0x80);
    delta.doors[0].attributes = 7;
    delta.doors[0].timeSinceTriggered = 100;
    delta.doors[0].state = 2;
    std::string opaque(200, '\0');
    opaque[74] = static_cast<char>(0xA5);
    delta.wire.replaceSection(MM6IndoorSectionKind::PersistentBytes, Blob::fromString(std::move(opaque)));

    Blob encoded = encodeMM6IndoorRuntimeDelta(delta, base);
    MM6IndoorRuntimeDelta decoded = decodeMM6IndoorRuntimeDelta(encoded, base);
    EXPECT_EQ(decoded.respawnCount, 3);
    EXPECT_EQ(decoded.lastRespawnDay, 42);
    EXPECT_EQ(static_cast<uint8_t>(decoded.visibleOutlines[0]), 0x80);
    EXPECT_EQ(decoded.doors[0].attributes, 7);
    EXPECT_EQ(decoded.doors[0].timeSinceTriggered, 100);
    EXPECT_EQ(decoded.doors[0].state, 2);
    EXPECT_EQ(static_cast<const uint8_t *>(decoded.wire.section(MM6IndoorSectionKind::PersistentBytes).bytes.data())[74], 0xA5);

    delta.doors[0].moveLength = 1;
    EXPECT_THROW(encodeMM6IndoorRuntimeDelta(delta, base), Exception);
    delta.doors[0].moveLength = 0;
    delta.doors.pop_back();
    EXPECT_THROW(encodeMM6IndoorRuntimeDelta(delta, base), Exception);
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
    bool testedBadDoorReference = false;
    bool testedActorWrite = false;
    bool testedGroundWrite = false;
    bool testedChestWrite = false;
    bool testedDoorWrite = false;
    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
        if (entry.path().extension() != ".blv")
            continue;
        Blob baseBytes = readFile(entry.path());
        Blob retainedEntry = readFile(entry.path().parent_path().parent_path() / "Games" / entry.path().filename());
        expectEqualBytes(baseBytes, decodeMM6IndoorBaseArchiveEntry(entry.path().filename().string(), retainedEntry));
        if (entry.path().filename() == "cd3.blv" || entry.path().filename() == "d08.blv")
            EXPECT_THROW(decodeMM6IndoorBaseArchiveEntry("other.blv", retainedEntry), Exception);
        std::filesystem::path deltaPath = entry.path();
        deltaPath.replace_extension(".dlv");
        Blob deltaBytes = readFile(deltaPath);
        MM6IndoorSnapshot base = deserializeMM6IndoorBase(baseBytes);
        MM6IndoorSnapshot delta = deserializeMM6IndoorDelta(deltaBytes, base);
        IndoorLocation_MM7 projected = projectMM6IndoorBase(base);
        MM6IndoorRuntimeDelta runtimeDelta = decodeMM6IndoorRuntimeDelta(deltaBytes, base);
        expectEqualBytes(deltaBytes, encodeMM6IndoorRuntimeDelta(runtimeDelta, base));
        EXPECT_EQ(projected.faces.size(), base.section(MM6IndoorSectionKind::Faces).count);
        EXPECT_EQ(projected.sectors.size(), base.section(MM6IndoorSectionKind::Rooms).count);
        EXPECT_EQ(projected.lights.size(), base.section(MM6IndoorSectionKind::Lights).count);
        EXPECT_EQ(projected.decorations.size(), base.section(MM6IndoorSectionKind::Sprites).count);
        EXPECT_EQ(runtimeDelta.actors.size(), delta.section(MM6IndoorSectionKind::Actors).count);
        EXPECT_EQ(runtimeDelta.spriteObjects.size(), delta.section(MM6IndoorSectionKind::GroundItems).count);
        EXPECT_EQ(runtimeDelta.chests.size(), delta.section(MM6IndoorSectionKind::Chests).count);
        EXPECT_EQ(runtimeDelta.doors.size(), 200);
        expectEqualBytes(deltaBytes, serializeMM6Indoor(runtimeDelta.wire));
        expectEqualBytes(baseBytes, serializeMM6Indoor(base));
        expectEqualBytes(deltaBytes, serializeMM6Indoor(delta));

        if (!testedActorWrite && !runtimeDelta.actors.empty()) {
            MM6IndoorRuntimeDelta edited = decodeMM6IndoorRuntimeDelta(deltaBytes, base);
            edited.actors[0].attributes ^= 1;
            edited.actors[0].hp += 1;
            MM6IndoorRuntimeDelta reopened = decodeMM6IndoorRuntimeDelta(encodeMM6IndoorRuntimeDelta(edited, base), base);
            EXPECT_EQ(reopened.actors[0].attributes, edited.actors[0].attributes);
            EXPECT_EQ(reopened.actors[0].hp, edited.actors[0].hp);
            expectEqualBytes(delta.section(MM6IndoorSectionKind::PersistentBytes).bytes,
                             reopened.wire.section(MM6IndoorSectionKind::PersistentBytes).bytes);
            edited.actors[0].hp = -1;
            EXPECT_THROW(encodeMM6IndoorRuntimeDelta(edited, base), Exception);
            testedActorWrite = true;
        }
        if (!testedGroundWrite && !runtimeDelta.spriteObjects.empty()) {
            MM6IndoorRuntimeDelta edited = decodeMM6IndoorRuntimeDelta(deltaBytes, base);
            edited.spriteObjects[0].position.x += 1;
            edited.spriteObjects[0].uAttributes ^= 1;
            edited.spriteObjects[0].containing_item.flags ^= 1;
            MM6IndoorRuntimeDelta reopened = decodeMM6IndoorRuntimeDelta(encodeMM6IndoorRuntimeDelta(edited, base), base);
            EXPECT_EQ(reopened.spriteObjects[0].position.x, edited.spriteObjects[0].position.x);
            EXPECT_EQ(reopened.spriteObjects[0].uAttributes, edited.spriteObjects[0].uAttributes);
            EXPECT_EQ(reopened.spriteObjects[0].containing_item.flags, edited.spriteObjects[0].containing_item.flags);
            edited.spriteObjects[0].uSpellID += 1;
            EXPECT_THROW(encodeMM6IndoorRuntimeDelta(edited, base), Exception);
            testedGroundWrite = true;
        }
        if (!testedChestWrite && !runtimeDelta.chests.empty()) {
            MM6IndoorRuntimeDelta edited = decodeMM6IndoorRuntimeDelta(deltaBytes, base);
            edited.chests[0].flags ^= 1;
            edited.chests[0].items[0].flags ^= 1;
            MM6IndoorRuntimeDelta reopened = decodeMM6IndoorRuntimeDelta(encodeMM6IndoorRuntimeDelta(edited, base), base);
            EXPECT_EQ(reopened.chests[0].flags, edited.chests[0].flags);
            EXPECT_EQ(reopened.chests[0].items[0].flags, edited.chests[0].items[0].flags);
            expectEqualBytes(delta.section(MM6IndoorSectionKind::PersistentBytes).bytes,
                             reopened.wire.section(MM6IndoorSectionKind::PersistentBytes).bytes);
            edited.chests[0].inventoryMatrix[0] ^= 1;
            EXPECT_THROW(encodeMM6IndoorRuntimeDelta(edited, base), Exception);
            testedChestWrite = true;
        }
        if (!testedDoorWrite && !runtimeDelta.doorsData.empty()) {
            MM6IndoorRuntimeDelta edited = decodeMM6IndoorRuntimeDelta(deltaBytes, base);
            edited.doors[0].attributes ^= 1;
            edited.doors[0].timeSinceTriggered += 1;
            MM6IndoorRuntimeDelta reopened = decodeMM6IndoorRuntimeDelta(encodeMM6IndoorRuntimeDelta(edited, base), base);
            EXPECT_EQ(reopened.doors[0].attributes, edited.doors[0].attributes);
            EXPECT_EQ(reopened.doors[0].timeSinceTriggered, edited.doors[0].timeSinceTriggered);
            expectEqualBytes(delta.section(MM6IndoorSectionKind::DoorData).bytes,
                             reopened.wire.section(MM6IndoorSectionKind::DoorData).bytes);
            testedDoorWrite = true;
        }

        if (!testedBadDoorReference && !runtimeDelta.doorsData.empty()) {
            Blob originalDoorData = Blob::copy(delta.section(MM6IndoorSectionKind::DoorData).bytes);
            std::string badData(static_cast<const char *>(originalDoorData.data()), originalDoorData.size());
            badData[0] = static_cast<char>(0xFF);
            badData[1] = static_cast<char>(0x7F);
            delta.replaceSection(MM6IndoorSectionKind::DoorData, Blob::fromString(std::move(badData)));
            EXPECT_THROW(decodeMM6IndoorRuntimeDelta(serializeMM6Indoor(delta), base), Exception);
            delta.replaceSection(MM6IndoorSectionKind::DoorData, originalDoorData);
            testedBadDoorReference = true;
        }

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
    EXPECT_TRUE(testedBadDoorReference);
    EXPECT_TRUE(testedActorWrite);
    EXPECT_TRUE(testedGroundWrite);
    EXPECT_TRUE(testedChestWrite);
    EXPECT_TRUE(testedDoorWrite);
}
