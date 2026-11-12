#include "MM6OutdoorSky.h"

#include <array>
#include <string>

#include "Core/Time/Time.h"

#include "Library/Random/RandomEngine.h"

#include "Utility/String/Format.h"

#include "MM6MapVisit.h"

namespace mm6 {

// MM6.exe sky tables.
static constexpr std::array<int, 9> commonSkies = {{1, 3, 6, 7, 8, 9, 12, 14, 15}};
static constexpr std::array<int, 7> rareSkies = {{2, 5, 10, 13, 16, 18, 19}};

std::string outdoorSky(std::span<const std::uint8_t> deltaBytes, int dayOfMonth, RandomEngine *rng) {
    const Time lastVisit = mapLastVisit(deltaBytes);
    if (!lastVisit)
        return "sky01";

    if (lastVisit.toDays() % 28 == dayOfMonth)
        return mapVisitSky(deltaBytes);

    if (rng->random(100) < 20)
        return fmt::format("sky{:02}", rareSkies[rng->random(rareSkies.size())]);
    return fmt::format("sky{:02}", commonSkies[rng->random(commonSkies.size())]);
}

} // namespace mm6
