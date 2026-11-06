#pragma once

#include <optional>

namespace mm6 {

// Pure numeric checks for raw EVT variable 21. The MM6 event path applies
// the checked result silently while title-specific presentation is unresolved.
bool hasEventGold(int balance, int amount);
int goldAfterEventAdd(int balance, int amount);
std::optional<int> goldAfterEventSubtract(int balance, int amount);

} // namespace mm6
