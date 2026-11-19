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

MapTimerFirings &mapTimerFirings() {
    static MapTimerFirings firings;
    return firings;
}

int mapTimerIntervalIndex(Duration interval) {
    if (interval == Duration::fromYears(1))
        return 0;
    if (interval == Duration::fromDays(28))
        return 1;
    if (interval == Duration::fromDays(7))
        return 2;
    assert(interval == Duration::fromDays(1));
    return 3;
}

Time mapTimerFirstAlarmAfterLastFiring(EvtOpcode trigger, Duration interval, Duration timeInsideDay, Time lastVisit, Time lastFiring,
                                       Time now) {
    assert(trigger == EVENT_OnTimer || trigger == EVENT_OnLongTimer);
    if (trigger == EVENT_OnLongTimer) {
        if (!lastVisit)
            return Time();
        if (!lastFiring || lastVisit - lastFiring > interval)
            lastFiring = lastVisit; // A save without TIMR, or a timer the map's script didn't have on the last visit.
        return lastFiring + interval;
    }
    if (interval == Duration::fromDays(1)) {
        const Time today = Time::fromDays(now.toDays()) + timeInsideDay;
        return today > now ? today : today + interval;
    }
    return now + interval;
}

Time longTimerNextAlarm(Time alarm, Duration interval, Time now) {
    const Time next = alarm + interval;
    return next > now ? next : now + interval;
}

} // namespace mm6
