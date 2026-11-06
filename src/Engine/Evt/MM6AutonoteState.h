#pragma once

#include "Utility/IndexedBitset.h"

// MM6 event variable 205 addresses the party's 128 shipped autonote rows.
// The wider 208-bit runtime field remains untouched outside those rows and is
// already carried by the title-specific QEST save section.
class MM6AutonoteState {
 public:
    explicit MM6AutonoteState(IndexedBitset<1, 208> &bits) : _bits(bits) {}

    bool compare(int rawId, int value) const;
    bool add(int rawId, int value);
    bool subtract(int rawId, int value);
    bool set(int rawId, int value);

 private:
    IndexedBitset<1, 208> &_bits;

    static int checkedIndex(int rawId, int value);
};
