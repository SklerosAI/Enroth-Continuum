#include "MM6OutdoorSky.h"

#include <cassert>
#include <cstring>
#include <array>
#include <string>

#include "Core/Time/Time.h"

#include "Library/Random/RandomEngine.h"

#include "Utility/String/Format.h"

namespace mm6 {

static constexpr size_t VISIT_BLOCK_SIZE = 56; // Last visit time, then the 48-byte weather block.
static constexpr size_t SKY_NAME_SIZE = 12;

// MM6.exe sky tables.
static constexpr std::array<int, 9> commonSkies = {{1, 3, 6, 7, 8, 9, 12, 14, 15}};
static constexpr std::array<int, 7> rareSkies = {{2, 5, 10, 13, 16, 18, 19}};

std::string outdoorSky(std::span<const std::uint8_t> deltaBytes, int dayOfMonth, RandomEngine *rng) {
    assert(deltaBytes.size() >= VISIT_BLOCK_SIZE);
    const std::span<const std::uint8_t> visit = deltaBytes.last(VISIT_BLOCK_SIZE);

    std::int64_t lastVisitTicks = 0;
    std::memcpy(&lastVisitTicks, visit.data(), sizeof(lastVisitTicks));
    if (lastVisitTicks == 0)
        return "sky01";

    if (Time::fromTicks(lastVisitTicks).toDays() % 28 == dayOfMonth) {
        const char *name = reinterpret_cast<const char *>(visit.data() + sizeof(lastVisitTicks));
        return std::string(name, strnlen(name, SKY_NAME_SIZE));
    }

    if (rng->random(100) < 20)
        return fmt::format("sky{:02}", rareSkies[rng->random(rareSkies.size())]);
    return fmt::format("sky{:02}", commonSkies[rng->random(commonSkies.size())]);
}

} // namespace mm6
