#pragma once

#include "Engine/Objects/Character.h"
#include "Engine/Objects/Item.h"

namespace mm6 {

// Read-only raw EVT variable 17 comparison. Shipped MM6 item IDs occupy
// 1..580; acquiring and consuming items need separate title-specific rules.
bool hasEventItem(const Character &character, const Item &heldItem, int rawId, int value);

} // namespace mm6
