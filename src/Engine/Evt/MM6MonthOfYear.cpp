#include "Engine/Evt/MM6MonthOfYear.h"

#include <stdexcept>

namespace mm6 {

bool compareMonthOfYear(int currentMonth, int rawId, int value) {
    if (rawId != 226 || value < 0 || value > 11 || currentMonth < 0 || currentMonth > 11)
        throw std::runtime_error("MM6 month-of-year event operand or cache is invalid");
    return currentMonth == value;
}

} // namespace mm6
