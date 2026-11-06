#include "Engine/Evt/MM6CharacterEventBit.h"

#include <stdexcept>

namespace mm6 {

int CharacterEventBitState::checkedBit(int rawId, int value) {
    if (rawId != 213 || value < 0 || value >= 512)
        throw std::runtime_error("MM6 character event bit variable or operand is invalid");
    return value + 1;
}

bool CharacterEventBitState::compare(int rawId, int value) const {
    return _bits.test(checkedBit(rawId, value));
}

void CharacterEventBitState::set(int rawId, int value) {
    _bits.set(checkedBit(rawId, value));
}

} // namespace mm6
