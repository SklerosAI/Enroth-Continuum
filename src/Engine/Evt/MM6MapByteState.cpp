#include "Engine/Evt/MM6MapByteState.h"

#include <stdexcept>

namespace {
constexpr size_t kPersistentBytes = 200;
constexpr size_t kTailBytes = 56;
constexpr int kFirstEventId = 105;
constexpr int kLastActiveEventId = 175;
} // namespace

MM6MapByteState::MM6MapByteState(std::span<std::uint8_t> decodedDelta) {
    if (decodedDelta.size() < kPersistentBytes + kTailBytes)
        throw std::runtime_error("MM6 map delta is too short for persistent bytes");
    _bytes = decodedDelta.subspan(decodedDelta.size() - kPersistentBytes - kTailBytes, kPersistentBytes);
}

int MM6MapByteState::checkedIndex(int rawId) {
    if (rawId < kFirstEventId || rawId > kLastActiveEventId)
        throw std::runtime_error("MM6 map variable ID is outside the verified active range");
    return rawId - kFirstEventId;
}

std::uint8_t MM6MapByteState::checkedValue(int value) {
    if (value < 0 || value > 255)
        throw std::runtime_error("MM6 map variable operand is outside byte range");
    return static_cast<std::uint8_t>(value);
}

bool MM6MapByteState::compare(int rawId, int value) const {
    return _bytes[checkedIndex(rawId)] >= checkedValue(value);
}

void MM6MapByteState::add(int rawId, int value) {
    int index = checkedIndex(rawId);
    std::uint8_t operand = checkedValue(value);
    if (static_cast<int>(_bytes[index]) + operand > 255)
        throw std::runtime_error("MM6 map variable addition would overflow");
    _bytes[index] += operand;
}

void MM6MapByteState::subtract(int rawId, int value) {
    int index = checkedIndex(rawId);
    std::uint8_t operand = checkedValue(value);
    if (_bytes[index] < operand)
        throw std::runtime_error("MM6 map variable subtraction would underflow");
    _bytes[index] -= operand;
}

void MM6MapByteState::set(int rawId, int value) {
    _bytes[checkedIndex(rawId)] = checkedValue(value);
}
