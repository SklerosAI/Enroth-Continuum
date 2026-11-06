#pragma once

#include "Core/Time/Time.h"

namespace mm6 {

// Raw MM6 EVT variable 226 is MonthIs, a one-based calendar comparison.
bool compareMonthOfYear(Time playingTime, int rawId, int value);

} // namespace mm6
