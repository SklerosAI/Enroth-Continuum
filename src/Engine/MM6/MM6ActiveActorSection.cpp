#include "MM6ActiveActorSection.h"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "Engine/MM6/MM6SpawnedActorSection.h"
#include "Engine/Objects/Actor.h"

namespace mm6 {

std::vector<std::uint8_t> encodeActiveActorSection(
    std::span<const Actor> actors, GameTitle title) {
    std::vector<std::uint8_t> bytes = encodeSpawnedActorSection(actors, 0, title);
    std::copy_n("ACTR", 4, bytes.begin());
    return bytes;
}

std::vector<Actor> decodeActiveActorSection(
    std::span<const std::uint8_t> bytes, GameTitle title) {
    if (bytes.size() < 4 || !std::equal(bytes.begin(), bytes.begin() + 4, "ACTR"))
        throw std::runtime_error("invalid MM6 active actor section tag");
    std::vector<std::uint8_t> checked(bytes.begin(), bytes.end());
    std::copy_n("SPAW", 4, checked.begin());
    return decodeSpawnedActorSection(checked, 0, title);
}

} // namespace mm6
