#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <utility>

#include "MM6BoundedInventoryProjection.h"

namespace mm6 {

// Detached all-or-nothing view of the four physical MM6 character inventories.
// The raw record for each character remains in its individual projection.
class BoundedPartyInventoryProjection {
 public:
    CharacterInventory &inventory(std::size_t character) {
        return _characters.at(character).value().inventory();
    }

    const CharacterInventory &inventory(std::size_t character) const {
        return _characters.at(character).value().inventory();
    }

 private:
    BoundedPartyInventoryProjection() = default;
    friend std::optional<BoundedPartyInventoryProjection> stageBoundedPartyInventory(
        const RawInventoryFields &, const ItemTable &, GameTitle);
    friend std::optional<RawInventoryFields> captureBoundedPartyInventory(
        const BoundedPartyInventoryProjection &, const ItemTable &, GameTitle);

    std::array<std::optional<BoundedInventoryProjection>, 4> _characters;
};

inline std::optional<BoundedPartyInventoryProjection> stageBoundedPartyInventory(
    const RawInventoryFields &raw, const ItemTable &table, GameTitle title) {
    if (title != GameTitle::MM6 || !raw.ready || pItemTable != &table)
        return std::nullopt;

    BoundedPartyInventoryProjection result;
    for (std::size_t i = 0; i < result._characters.size(); ++i) {
        result._characters[i] = stageBoundedInventory(raw.characters[i], table, title);
        if (!result._characters[i])
            return std::nullopt;
    }
    return result;
}

inline std::optional<RawInventoryFields> captureBoundedPartyInventory(
    const BoundedPartyInventoryProjection &projection, const ItemTable &table, GameTitle title) {
    if (title != GameTitle::MM6 || pItemTable != &table)
        return std::nullopt;

    RawInventoryFields result;
    for (std::size_t i = 0; i < projection._characters.size(); ++i) {
        if (!projection._characters[i])
            return std::nullopt;
        auto captured = captureBoundedInventory(*projection._characters[i], table, title);
        if (!captured)
            return std::nullopt;
        result.characters[i] = std::move(*captured);
    }
    result.ready = true;
    return result;
}

} // namespace mm6
