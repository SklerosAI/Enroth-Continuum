#pragma once

#include "Core/Time/Time.h"

namespace mm6 {

/**
 * @param rawId                         MM6 EVT variable ID.
 * @return                              Whether `rawId` is one of the six MM6 day counters (216-221).
 */
bool isDayCounterVariable(int rawId);

/**
 * @param rawId                         MM6 day counter variable ID.
 * @return                              Index of the party event counter that stores the counter's start time.
 */
int dayCounterIndex(int rawId);

/**
 * Set and Add restart a day counter at the current time and Subtract clears it. None of them reads its operand.
 *
 * @param started                       Time the counter was last restarted, zero if it never was.
 * @param now                           Current playing time.
 * @param value                         Compare operand in days.
 * @return                              Whether at least `value` calendar days separate `started` and `now`.
 */
bool compareDayCounter(Time started, Time now, int value);

} // namespace mm6
