#include "Engine/Evt/MM6DayCounterEvent.h"

#include <stdexcept>

namespace mm6 {

bool isDayCounterVariable(int rawId) {
    return rawId >= 216 && rawId <= 221;
}

int dayCounterIndex(int rawId) {
    if (!isDayCounterVariable(rawId))
        throw std::runtime_error("MM6 day counter event variable is invalid");
    return rawId - 216;
}

bool compareDayCounter(Time started, Time now, int value) {
    if (started.ticks() < 0 || now.ticks() < 0)
        throw std::runtime_error("MM6 day counter event has a negative time");
    return now.toDays() - started.toDays() >= value;
}

} // namespace mm6
