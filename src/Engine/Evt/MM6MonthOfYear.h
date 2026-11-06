#pragma once

namespace mm6 {

// Raw MM6 EVT variable 226 is MonthIs and compares the zero-based cached
// party month with a zero-based operand.
bool compareMonthOfYear(int currentMonth, int rawId, int value);

} // namespace mm6
