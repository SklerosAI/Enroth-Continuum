#pragma once

#include "MM6RecoverySaveFields.h"

namespace mm6 {

// Apply an already decoded v2 RCVR owner to a detached Party copy after PART
// set the playing clock. The v1 byte path delegates here after decoding.
Party stageRecoveryParty(const Party &live, const RecoverySaveFields &fields, GameTitle title);

} // namespace mm6
