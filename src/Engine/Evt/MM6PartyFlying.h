#pragma once

namespace mm6 {

// Raw MM6 EVT variable 222 compares actual flight state. Its numeric Compare
// operand is ignored by the original executable.
bool comparePartyFlying(int rawId, int value, bool flying, bool flyBuffActive);

} // namespace mm6
