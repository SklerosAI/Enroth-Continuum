#include "MM6PersistentEventPublication.h"

#include <algorithm>
#include <stdexcept>
#include <type_traits>

namespace mm6 {
namespace {

constexpr std::size_t kPersistentBytes = 200;
constexpr std::size_t kTailBytes = 56;

} // namespace

void publishFreshChildPersistentEventZeros(
    PersistentVariables *destination,
    const DetachedPersistentEventBoundary &staged, GameTitle title) {
    static_assert(std::is_nothrow_copy_assignable_v<PersistentVariables>);
    if (!destination)
        throw std::runtime_error("MM6 event publication needs a fresh-child owner");
    // Validates title and refuses either unsupported generic owner before
    // changing the destination. The returned bytes remain in the sidecar.
    (void) capturePersistentEventBoundary(staged, title);
    *destination = staged.legacy;
}

void requirePostMapPersistentEvents(
    const DetachedPersistentEventBoundary &staged,
    std::span<const std::uint8_t> liveDecodedDelta,
    const PersistentVariables &liveLegacy, GameTitle title) {
    const auto expected = capturePersistentEventBoundary(staged, title);
    if (liveDecodedDelta.size() < kPersistentBytes + kTailBytes)
        throw std::runtime_error("MM6 reloaded delta lacks persistent event section");
    const auto live = liveDecodedDelta.subspan(
        liveDecodedDelta.size() - kPersistentBytes - kTailBytes, kPersistentBytes);
    if (!std::equal(live.begin(), live.end(), expected.begin()) ||
        std::any_of(liveLegacy.mapVars.begin(), liveLegacy.mapVars.end(),
                    [](unsigned char value) { return value != 0; }) ||
        std::any_of(liveLegacy.decorVars.begin(), liveLegacy.decorVars.end(),
                    [](unsigned char value) { return value != 0; }))
        throw std::runtime_error("MM6 post-map event owner differs from the staged bytes");
}

} // namespace mm6
