#pragma once

#include <cstdint>
#include <span>

/**
 * The MM6 map bytes, event variables 105 to 204, in a previously validated `.ddm` or `.dlv` delta. They are the first
 * 100 of the 200 persistent bytes that precede the 56-byte tail. Only 105 to 175 occur in shipped scripts, and only
 * those are accepted.
 */
class MM6MapByteState {
 public:
    explicit MM6MapByteState(std::span<std::uint8_t> decodedDelta);

    /**
     * @param rawId                     Event variable id.
     * @param value                     Operand, compared whole.
     * @return                          Whether the byte is at least `value`.
     */
    bool compare(int rawId, int value) const;

    /**
     * Adds the operand's low byte. The sum stops at 255.
     *
     * @param rawId                     Event variable id.
     * @param value                     Operand.
     */
    void add(int rawId, int value);

    /**
     * Subtracts the operand's low byte. The result wraps below 0.
     *
     * @param rawId                     Event variable id.
     * @param value                     Operand.
     */
    void subtract(int rawId, int value);

    /**
     * Stores the operand's low byte.
     *
     * @param rawId                     Event variable id.
     * @param value                     Operand.
     */
    void set(int rawId, int value);

 private:
    std::span<std::uint8_t> _bytes;

    static int checkedIndex(int rawId);
};
