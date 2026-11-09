#include "MM6Tavern.h"

#include <algorithm>

static int discountedPrice(int base, int merchant) {
    return std::max((100 - merchant) * base / 100, base / 3);
}

int mm6::tavernRoomPrice(float value, int merchant) {
    return discountedPrice(static_cast<int>(static_cast<double>(value) * value * 0.1f), merchant);
}

int mm6::tavernFoodPrice(float value, int merchant) {
    const float squared = value * value; // The original keeps the square as a float.
    return discountedPrice(static_cast<int>(static_cast<double>(squared) * value * 0.01f), merchant);
}
