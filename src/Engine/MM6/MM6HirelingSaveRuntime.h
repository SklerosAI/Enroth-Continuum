#pragma once

#include "MM6HirelingSaveFields.h"

namespace mm6 {

// Apply an already decoded v2 HIRE owner to a detached Party copy. The v1 byte
// path delegates here after decoding. This does not publish live Party state.
Party stageHirelingParty(const Party &live, const HirelingSaveFields &fields, GameTitle title);

} // namespace mm6
