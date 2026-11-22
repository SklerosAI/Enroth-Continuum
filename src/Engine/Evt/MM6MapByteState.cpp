#include "Engine/Evt/MM6MapByteState.h"

#include <algorithm>
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

bool MM6MapByteState::compare(int rawId, int value) const {
    return _bytes[checkedIndex(rawId)] >= value;
}

void MM6MapByteState::add(int rawId, int value) {
    std::uint8_t &byte = _bytes[checkedIndex(rawId)];
    byte = std::min(byte + (value & 0xFF), 255);
}

void MM6MapByteState::subtract(int rawId, int value) {
    std::uint8_t &byte = _bytes[checkedIndex(rawId)];
    byte -= value & 0xFF;
}

void MM6MapByteState::set(int rawId, int value) {
    _bytes[checkedIndex(rawId)] = static_cast<std::uint8_t>(value);
}
