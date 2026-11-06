#include "Engine/Evt/MM6PartyFlying.h"

#include <stdexcept>

namespace mm6 {

bool comparePartyFlying(int rawId, int value, bool flying, bool flyBuffActive) {
    if (rawId != 222)
        throw std::runtime_error("MM6 party-flying event variable is invalid");
    (void) value; // The original comparison does not read this operand.
    return flying && flyBuffActive;
}

} // namespace mm6
