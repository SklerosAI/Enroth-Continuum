#pragma once

#include <cstdint>
#include <span>

// Operates on a previously validated MM6 DDM/DLV delta sidecar. The 200-byte
// persistent section precedes the 56-byte tail in both title-specific codecs.
// Only MapVar0..70 (EVT IDs 105..175) have shipped-script evidence.
class MM6MapByteState {
 public:
    explicit MM6MapByteState(std::span<std::uint8_t> decodedDelta);

    bool compare(int rawId, int value) const;
    void add(int rawId, int value);
    void subtract(int rawId, int value);
    void set(int rawId, int value);

 private:
    std::span<std::uint8_t> _bytes;

    static int checkedIndex(int rawId);
    static std::uint8_t checkedValue(int value);
};
