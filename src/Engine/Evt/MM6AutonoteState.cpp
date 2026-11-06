#include "Engine/Evt/MM6AutonoteState.h"

#include <stdexcept>

int MM6AutonoteState::checkedIndex(int rawId, int value) {
    if (rawId != 205 || value < 1 || value > 128)
        throw std::runtime_error("MM6 autonote variable or operand is outside the shipped range");
    return value;
}

bool MM6AutonoteState::compare(int rawId, int value) const {
    return _bits.test(checkedIndex(rawId, value));
}

bool MM6AutonoteState::add(int rawId, int value) {
    const int index = checkedIndex(rawId, value);
    const bool changed = !_bits.test(index);
    _bits.set(index);
    return changed;
}

bool MM6AutonoteState::subtract(int rawId, int value) {
    const int index = checkedIndex(rawId, value);
    const bool changed = _bits.test(index);
    _bits.reset(index);
    return changed;
}

bool MM6AutonoteState::set(int rawId, int value) {
    return add(rawId, value);
}
