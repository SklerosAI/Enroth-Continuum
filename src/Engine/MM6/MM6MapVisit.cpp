#include "MM6MapVisit.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>

namespace mm6 {

static constexpr size_t VISIT_BLOCK_SIZE = 56;
static constexpr size_t SKY_OFFSET = 8;
static constexpr size_t SKY_NAME_SIZE = 12;

Time mapLastVisit(std::span<const std::uint8_t> deltaBytes) {
    assert(deltaBytes.size() >= VISIT_BLOCK_SIZE);
    std::int64_t ticks = 0;
    std::memcpy(&ticks, deltaBytes.last(VISIT_BLOCK_SIZE).data(), sizeof(ticks));
    return Time::fromTicks(ticks);
}

std::string mapVisitSky(std::span<const std::uint8_t> deltaBytes) {
    assert(deltaBytes.size() >= VISIT_BLOCK_SIZE);
    const char *name = reinterpret_cast<const char *>(deltaBytes.last(VISIT_BLOCK_SIZE).data() + SKY_OFFSET);
    return std::string(name, strnlen(name, SKY_NAME_SIZE));
}

void recordMapVisit(std::span<std::uint8_t> deltaBytes, Time leaveTime, std::string_view sky) {
    assert(deltaBytes.size() >= VISIT_BLOCK_SIZE);
    assert(sky.size() < SKY_NAME_SIZE);
    const std::span<std::uint8_t> visit = deltaBytes.last(VISIT_BLOCK_SIZE);
    const std::int64_t ticks = leaveTime.ticks();
    std::memcpy(visit.data(), &ticks, sizeof(ticks));
    if (sky.empty())
        return;
    const std::span<std::uint8_t> name = visit.subspan(SKY_OFFSET, SKY_NAME_SIZE);
    std::ranges::fill(name, 0);
    std::memcpy(name.data(), sky.data(), sky.size());
}

Time mapTimerFirstAlarm(EvtOpcode trigger, Duration interval, Duration timeInsideDay, Time lastVisit, Time now) {
    assert(trigger == EVENT_OnTimer || trigger == EVENT_OnLongTimer);
    if (trigger == EVENT_OnLongTimer && (!lastVisit || now - lastVisit >= Duration::fromDays(1)))
        return Time();
    if (interval == Duration::fromDays(1))
        return Time::fromDays(now.toDays() + 1) + timeInsideDay;
    return now + interval;
}

} // namespace mm6
