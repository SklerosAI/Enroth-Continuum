#pragma once

#include "Engine/GameTitle.h"

#include "Utility/Memory/Blob.h"

struct OutdoorLocation;

namespace mm6 {

// Capture the supported outdoor state while ACTR owns the full runtime actor
// list. Native DDM actor records, raw ground-item contents, chests and opaque
// sections stay byte-for-byte intact. Refuses any changed owner it cannot
// represent. Detached result; no map cache or profile is assigned.
Blob captureOutdoorDeltaWithActiveActors(const OutdoorLocation &location,
                                         GameTitle title);

} // namespace mm6
