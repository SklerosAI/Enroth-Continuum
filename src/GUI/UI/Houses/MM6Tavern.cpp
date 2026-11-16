#include "MM6Tavern.h"

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

#include "Library/Serialization/Serialization.h"

#include "Utility/String/Split.h"

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

int mm6::tavernRoomRestMinutes(int hour, int minute) {
    const int hoursToDawn = hour < 5 ? 5 - hour : 29 - hour; // 0x488BD0, the hours left until 5 am.
    return (hoursToDawn + 1) * 60 - minute;
}

std::vector<int> mm6::tavernNewsRegions(std::string_view npcNews) {
    std::vector<int> result;
    for (std::string_view line : split(npcNews).by("\r\n").drop(2).skip("")) {
        std::array<std::string_view, 4> cells = split(line).by('\t');
        result.push_back(fromString<int>(cells[1]));
    }
    return result;
}

std::pair<int, int> mm6::tavernTipNews(std::span<const int> regions, int region) {
    if (std::ranges::count(regions, region) == 0)
        region = 1;
    const int first = static_cast<int>(std::ranges::find(regions, region) - regions.begin());
    return {first, static_cast<int>(std::ranges::count(regions, region))};
}
