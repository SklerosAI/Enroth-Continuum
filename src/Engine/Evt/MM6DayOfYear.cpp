#include "Engine/Evt/MM6DayOfYear.h"

#include <stdexcept>

namespace mm6 {

bool compareDayOfYear(Time playingTime, int rawId, int value) {
    if (rawId != 19 || value < 1 || value > 336 || playingTime.ticks() < 0)
        throw std::runtime_error("MM6 day-of-year event operand or time is invalid");
    return playingTime.toDays() % 336 + 1 == value;
}

} // namespace mm6
