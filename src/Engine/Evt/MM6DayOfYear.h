#pragma once

#include "Core/Time/Time.h"

namespace mm6 {

// Raw EVT variable 19 is the one-based day in a 336-day game year.
// The shipped MM6 scripts use it only in comparisons.
bool compareDayOfYear(Time playingTime, int rawId, int value);

} // namespace mm6
