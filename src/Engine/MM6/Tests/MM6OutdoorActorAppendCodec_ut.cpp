#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <span>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6OutdoorActorAppendCodec.h"
#include "Engine/Objects/Actor.h"

namespace {

constexpr size_t VISIBILITY_BYTES = 2 * 88 * 11;
constexpr size_t ACTOR_BYTES = mm6::MM6_OUTDOOR_ACTOR_RECORD_BYTES;

void write16(std::span<std::uint8_t> bytes, size_t offset, std::uint16_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value);
    bytes[offset + 1] = static_cast<std::uint8_t>(value >> 8);
}

std::array<std::uint8_t, ACTOR_BYTES> record(int monsterId, int x) {
    std::array<std::uint8_t, ACTOR_BYTES> result{};
    result.fill(0xA5);
    result[52] = 7;
    write16(result, 118, monsterId);
    write16(result, 146, x);
    write16(result, 148, 20);
    write16(result, 150, 30);
    return result;
}

Actor actor(int slot, int monsterId, int x) {
    Actor result(slot);
    result.monsterInfo.id = static_cast<MonsterId>(7);
    result.monsterId = static_cast<MonsterId>(monsterId);
    result.initialPosition = Vec3f(x, 20, 30);
    return result;
}

std::vector<std::uint8_t> sourceBytes(std::span<const std::uint8_t> originalActor) {
    std::vector<std::uint8_t> bytes(8 + VISIBILITY_BYTES + 4 + ACTOR_BYTES +
                                    4 + 4 + 200 + 56, 0);
    bytes[0] = 3; // Preserve the original respawn count.
    std::fill(bytes.begin() + 8, bytes.begin() + 8 + VISIBILITY_BYTES, 0x31);
    const size_t actorOffset = 8 + VISIBILITY_BYTES;
    bytes[actorOffset] = 1;
    std::copy(originalActor.begin(), originalActor.end(), bytes.begin() + actorOffset + 4);
    std::fill(bytes.begin() + actorOffset + 4 + ACTOR_BYTES, bytes.end(), 0x57);
    // Ground-item and chest counts stay zero; all later data is opaque.
    std::fill_n(bytes.begin() + actorOffset + 4 + ACTOR_BYTES, 8, 0);
    return bytes;
}

} // namespace

UNIT_TEST(MM6OutdoorActorAppendCodec, PreservesOriginalRecordsAndOpaqueSuffix) {
    const auto originalRecord = record(12, 10);
    const auto appendedRecord = record(13, 40);
    const std::vector<std::uint8_t> source = sourceBytes(originalRecord);
    const mm6::OutdoorDeltaWire decoded = mm6::decodeOutdoorDelta(source);
    std::array<Actor, 2> live = {actor(0, 12, 10), actor(1, 13, 40)};
    const mm6::OutdoorDeltaWire result = mm6::appendMM6OutdoorActorRecords(
        decoded, live, appendedRecord, 5, 1000, GameTitle::MM6);
    EXPECT_EQ(result.actorCount, 2);
    EXPECT_EQ(result.bytes.size(), source.size() + ACTOR_BYTES);
    EXPECT_EQ(result.part(mm6::OutdoorDeltaPart::ResetHeader)[0], 3);
    EXPECT_EQ(result.part(mm6::OutdoorDeltaPart::ResetHeader)[4], 5);
    const auto records = result.part(mm6::OutdoorDeltaPart::Actors).subspan(4);
    EXPECT_TRUE(std::equal(originalRecord.begin(), originalRecord.end(), records.begin()));
    EXPECT_TRUE(std::equal(appendedRecord.begin(), appendedRecord.end(), records.begin() + ACTOR_BYTES));
    const size_t originalSuffix = decoded.parts[static_cast<size_t>(mm6::OutdoorDeltaPart::GroundItems)].offset;
    EXPECT_TRUE(std::equal(source.begin() + originalSuffix, source.end(),
                           result.bytes.begin() + originalSuffix + ACTOR_BYTES));
    EXPECT_EQ(mm6::encodeOutdoorDelta(result), result.bytes);
    EXPECT_EQ(decoded.bytes, source);
}

UNIT_TEST(MM6OutdoorActorAppendCodec, RejectsReusedOrMismatchedSlots) {
    const auto originalRecord = record(12, 10);
    const auto appendedRecord = record(13, 40);
    const mm6::OutdoorDeltaWire source = mm6::decodeOutdoorDelta(sourceBytes(originalRecord));
    std::array<Actor, 2> live = {actor(0, 12, 10), actor(1, 13, 40)};
    live[0].monsterId = static_cast<MonsterId>(14);
    EXPECT_THROW((void) mm6::appendMM6OutdoorActorRecords(
        source, live, appendedRecord, 5, 1000, GameTitle::MM6), std::exception);
    live = {actor(0, 12, 10), actor(1, 13, 40)};
    live[1].id = 0;
    EXPECT_THROW((void) mm6::appendMM6OutdoorActorRecords(
        source, live, appendedRecord, 5, 1000, GameTitle::MM6), std::exception);
    live = {actor(0, 12, 10), actor(1, 13, 40)};
    EXPECT_THROW((void) mm6::appendMM6OutdoorActorRecords(
        source, live, originalRecord, 5, 1000, GameTitle::MM6), std::exception);
}

UNIT_TEST(MM6OutdoorActorAppendCodec, RejectsUnboundedOrNoninitialSplice) {
    const auto originalRecord = record(12, 10);
    const auto appendedRecord = record(13, 40);
    mm6::OutdoorDeltaWire source = mm6::decodeOutdoorDelta(sourceBytes(originalRecord));
    std::array<Actor, 2> live = {actor(0, 12, 10), actor(1, 13, 40)};
    EXPECT_THROW((void) mm6::appendMM6OutdoorActorRecords(
        source, live, appendedRecord, 0, 1000, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::appendMM6OutdoorActorRecords(
        source, live, appendedRecord, 5, 1, GameTitle::MM6), std::exception);
    EXPECT_THROW((void) mm6::appendMM6OutdoorActorRecords(
        source, live, appendedRecord, 5, 1000, GameTitle::MM7), std::exception);
    EXPECT_THROW((void) mm6::appendMM6OutdoorActorRecords(
        source, live, std::span(appendedRecord).subspan(1), 5, 1000, GameTitle::MM6), std::exception);
    source.part(mm6::OutdoorDeltaPart::ResetHeader)[4] = 2;
    EXPECT_THROW((void) mm6::appendMM6OutdoorActorRecords(
        source, live, appendedRecord, 5, 1000, GameTitle::MM6), std::exception);
}
