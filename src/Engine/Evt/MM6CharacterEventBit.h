#pragma once

#include "Utility/IndexedBitset.h"

namespace mm6 {

// Raw MM6 EVT variable 213 addresses zero-based positions in a 512-bit saved
// field. Runtime IndexedBitset keys are one-based, so operands shift by one.
// Only Compare and Set occur in active scripts.
class CharacterEventBitState {
 public:
    explicit CharacterEventBitState(IndexedBitset<1, 512> &bits) : _bits(bits) {}

    bool compare(int rawId, int value) const;
    void set(int rawId, int value);

 private:
    IndexedBitset<1, 512> &_bits;

    static int checkedBit(int rawId, int value);
};

} // namespace mm6
