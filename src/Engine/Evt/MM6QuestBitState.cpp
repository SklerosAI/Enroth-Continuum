#include "Engine/Evt/MM6QuestBitState.h"

#include <stdexcept>

QuestBit MM6QuestBitState::checkedBit(int rawId, int value) {
    if (rawId != 16 || value < 1 || value > 512)
        throw std::runtime_error("MM6 quest-bit variable or operand is outside the title range");
    return static_cast<QuestBit>(value);
}

bool MM6QuestBitState::compare(int rawId, int value) const {
    return _bits.test(checkedBit(rawId, value));
}

bool MM6QuestBitState::add(int rawId, int value) {
    const QuestBit bit = checkedBit(rawId, value);
    const bool changed = !_bits.test(bit);
    _bits.set(bit);
    return changed;
}

bool MM6QuestBitState::subtract(int rawId, int value) {
    const QuestBit bit = checkedBit(rawId, value);
    const bool changed = _bits.test(bit);
    _bits.reset(bit);
    return changed;
}

bool MM6QuestBitState::set(int rawId, int value) {
    return add(rawId, value);
}
