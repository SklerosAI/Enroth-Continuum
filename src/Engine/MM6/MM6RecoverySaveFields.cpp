#include "MM6RecoverySaveFields.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#include "Core/Time/Time.h"

#include "MM6Calendar.h"
#include "MM6CoreSaveFields.h"

namespace mm6 {
namespace {

constexpr std::array<char, 4> TAG = {'R', 'C', 'V', 'R'};
constexpr size_t PAYLOAD_SIZE = 4 + 8 + 8 + 4 + 4 * 8 + 4 * 8 + 4 * 3 + 59 * 8 + 8 + 4 + 4;

void put32(std::vector<std::uint8_t> *bytes, std::int32_t value) {
    std::uint32_t raw = std::bit_cast<std::uint32_t>(value);
    for (int shift = 0; shift < 32; shift += 8)
        bytes->push_back(raw >> shift);
}

std::int32_t get32(std::span<const std::uint8_t> bytes, size_t *offset) {
    std::uint32_t raw = 0;
    for (int shift = 0; shift < 32; shift += 8)
        raw |= std::uint32_t(bytes[(*offset)++]) << shift;
    return std::bit_cast<std::int32_t>(raw);
}

void put64(std::vector<std::uint8_t> *bytes, std::int64_t value) {
    std::uint64_t raw = std::bit_cast<std::uint64_t>(value);
    for (int shift = 0; shift < 64; shift += 8)
        bytes->push_back(raw >> shift);
}

std::int64_t get64(std::span<const std::uint8_t> bytes, size_t *offset) {
    std::uint64_t raw = 0;
    for (int shift = 0; shift < 64; shift += 8)
        raw |= std::uint64_t(bytes[(*offset)++]) << shift;
    return std::bit_cast<std::int64_t>(raw);
}

void validateClock(std::int64_t ticks) {
    if (ticks < 0 || ticks > Time::fromYears(10000).ticks())
        throw std::runtime_error("MM6 recovery clock exceeds supported range");
}

void validate(const RecoverySaveFields &fields) {
    validateClock(fields.playingTicks);
    if (fields.lastRegenerationTicks < 0 || fields.lastRegenerationTicks > fields.playingTicks)
        throw std::runtime_error("invalid MM6 last-regeneration clock");
    for (std::int64_t ticks : fields.characterRecoveryTicks)
        if (ticks < 0)
            throw std::runtime_error("negative MM6 character recovery time");
    for (std::int64_t ticks : fields.turnBasedRecoveryTicks)
        if (ticks < 0)
            throw std::runtime_error("negative MM6 turn-based recovery time");
    for (const RecoveryDailyCounters &counters : fields.daily)
        if (counters.divineIntervention > 127 || counters.armageddon > 127 ||
            counters.fireSpike > 127)
            throw std::runtime_error("MM6 daily spell counter exceeds runtime char range");
    auto validateEventClocks = [](const auto &clocks) {
        for (std::int64_t ticks : clocks)
            if (ticks < -1 || ticks > Time::fromYears(10000).ticks())
                throw std::runtime_error("MM6 event clock exceeds supported range");
    };
    validateEventClocks(fields.eventClocks.counters);
    validateEventClocks(fields.eventClocks.history);
    validateEventClocks(fields.eventClocks.unknownTimes);
    if (fields.armageddonTimerTicks < 0 || fields.armageddonTimerTicks > 417)
        throw std::runtime_error("MM6 Armageddon timer exceeds current-runtime range");
}

} // namespace

SaveSection encodeRecoverySection(const RecoverySaveFields &fields) {
    validate(fields);
    std::vector<std::uint8_t> bytes;
    bytes.reserve(PAYLOAD_SIZE);
    bytes.insert(bytes.end(), {3, 4, 0, 0}); // Version, character count, reserved.
    put64(&bytes, fields.playingTicks);
    put64(&bytes, fields.lastRegenerationTicks);
    bytes.insert(bytes.end(), {fields.daysWithoutRest, 0, 0, 0});
    for (std::int64_t ticks : fields.characterRecoveryTicks)
        put64(&bytes, ticks);
    for (std::int64_t ticks : fields.turnBasedRecoveryTicks)
        put64(&bytes, ticks);
    for (const RecoveryDailyCounters &counters : fields.daily) {
        bytes.push_back(counters.divineIntervention);
        bytes.push_back(counters.armageddon);
        bytes.push_back(counters.fireSpike);
    }
    for (std::int64_t ticks : fields.eventClocks.counters)
        put64(&bytes, ticks);
    for (std::int64_t ticks : fields.eventClocks.history)
        put64(&bytes, ticks);
    for (std::int64_t ticks : fields.eventClocks.unknownTimes)
        put64(&bytes, ticks);
    put64(&bytes, fields.armageddonTimerTicks);
    put32(&bytes, fields.armageddonDamage);
    put32(&bytes, fields.armageddonForceCount);
    return {TAG, true, std::move(bytes)};
}

RecoverySaveFields decodeRecoverySection(const SaveContainer &container) {
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections) {
        if (candidate.tag == TAG) {
            if (section)
                throw std::runtime_error("duplicate MM6 RCVR section");
            section = &candidate;
        }
    }
    if (!section || !section->required || section->payload.size() != PAYLOAD_SIZE)
        throw std::runtime_error("missing or invalid MM6 RCVR section");
    std::span<const std::uint8_t> bytes = section->payload;
    if (bytes[0] != 3 || bytes[1] != 4 || bytes[2] || bytes[3] ||
        bytes[21] || bytes[22] || bytes[23])
        throw std::runtime_error("unsupported MM6 RCVR version or reserved fields");
    RecoverySaveFields result;
    size_t offset = 4;
    result.playingTicks = get64(bytes, &offset);
    result.lastRegenerationTicks = get64(bytes, &offset);
    result.daysWithoutRest = bytes[offset++];
    offset += 3; // Already checked reserved bytes above.
    for (std::int64_t &ticks : result.characterRecoveryTicks)
        ticks = get64(bytes, &offset);
    for (std::int64_t &ticks : result.turnBasedRecoveryTicks)
        ticks = get64(bytes, &offset);
    for (RecoveryDailyCounters &counters : result.daily) {
        counters.divineIntervention = bytes[offset++];
        counters.armageddon = bytes[offset++];
        counters.fireSpike = bytes[offset++];
    }
    for (std::int64_t &ticks : result.eventClocks.counters)
        ticks = get64(bytes, &offset);
    for (std::int64_t &ticks : result.eventClocks.history)
        ticks = get64(bytes, &offset);
    for (std::int64_t &ticks : result.eventClocks.unknownTimes)
        ticks = get64(bytes, &offset);
    result.armageddonTimerTicks = get64(bytes, &offset);
    result.armageddonDamage = get32(bytes, &offset);
    result.armageddonForceCount = get32(bytes, &offset);
    validate(result);
    return result;
}

void loadRecoveryBeforeMutation(std::span<const std::uint8_t> bytes,
                                RecoverySaveFields *destination) {
    if (!destination)
        throw std::runtime_error("null MM6 recovery destination");
    RecoverySaveFields staged = decodeRecoverySection(decodeSaveContainer(bytes));
    *destination = std::move(staged);
}

RecoveryCalendar calendarForPlayingTicks(std::int64_t playingTicks) {
    validateClock(playingTicks);
    CivilTime civil = Time::fromTicks(playingTicks).toCivilTime(calendarStartingYear);
    return {civil.year, civil.month - 1, civil.week - 1, civil.day - 1,
            civil.hour, civil.minute, civil.second};
}

void validateRecoveryAgainstCore(const RecoverySaveFields &recovery, const CoreSaveFields &core) {
    validate(recovery);
    if (recovery.playingTicks != core.party.playingTicks)
        throw std::runtime_error("MM6 RCVR clock disagrees with PART");
}

} // namespace mm6
