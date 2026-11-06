#include "Engine/Evt/MM6MonthOfYear.h"

#include <stdexcept>

namespace mm6 {

bool compareMonthOfYear(Time playingTime, int rawId, int value) {
    if (rawId != 226 || value < 1 || value > 12 || playingTime.ticks() < 0)
        throw std::runtime_error("MM6 month-of-year event operand or time is invalid");
    return playingTime.toCivilTime().month == value;
}

} // namespace mm6
