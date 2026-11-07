#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <variant>

#include "Engine/Engine.h"

#include "MM6MapSaveValidation.h"

namespace mm6 {

// MM6 event IDs 105..175 use only bytes 0..70 of the retained DLV/DDM
// persistent section. The relation of its other 129 bytes to the inherited
// 75/125 live arrays is unproved. Keep the known bytes in the map sidecar and
// admit a zero legacy owner only while all unproved bytes remain zero.
struct DetachedPersistentEventBoundary {
    std::array<std::uint8_t, 71> mapBytes{};
    PersistentVariables legacy{};
};

inline DetachedPersistentEventBoundary stagePersistentEventBoundary(
    const StagedActiveMap &activeMap, GameTitle title) {
    static_assert(sizeof(PersistentVariables{}.mapVars) == 75);
    static_assert(sizeof(PersistentVariables{}.decorVars) == 125);
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 persistent event stage requires MM6 title");
    const std::span<const std::uint8_t> bytes = std::visit([](const auto &map) -> std::span<const std::uint8_t> {
        if constexpr (std::is_same_v<std::decay_t<decltype(map)>, StagedIndoorMap>) {
            const Blob &section = map.delta.wire.section(MM6IndoorSectionKind::PersistentBytes).bytes;
            return {static_cast<const std::uint8_t *>(section.data()), section.size()};
        } else {
            return map.delta.wire.part(OutdoorDeltaPart::PersistentBytes);
        }
    }, activeMap);
    if (bytes.size() != 200 ||
        std::any_of(bytes.begin() + 71, bytes.end(), [](std::uint8_t value) { return value != 0; }))
        throw std::runtime_error("MM6 persistent event stage has unproved nonzero bytes");
    DetachedPersistentEventBoundary staged;
    std::copy_n(bytes.begin(), staged.mapBytes.size(), staged.mapBytes.begin());
    return staged;
}

inline std::array<std::uint8_t, 200> capturePersistentEventBoundary(
    const DetachedPersistentEventBoundary &staged, GameTitle title) {
    if (title != GameTitle::MM6 ||
        std::any_of(staged.legacy.mapVars.begin(), staged.legacy.mapVars.end(),
                    [](std::uint8_t value) { return value != 0; }) ||
        std::any_of(staged.legacy.decorVars.begin(), staged.legacy.decorVars.end(),
                    [](std::uint8_t value) { return value != 0; }))
        throw std::runtime_error("MM6 persistent event capture has unproved live state");
    std::array<std::uint8_t, 200> bytes{};
    std::copy(staged.mapBytes.begin(), staged.mapBytes.end(), bytes.begin());
    return bytes;
}

} // namespace mm6
