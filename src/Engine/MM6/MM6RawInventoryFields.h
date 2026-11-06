#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "Engine/GameTitle.h"

#include "MM6ItemRecord28.h"
#include "MM6SaveContainer.h"

namespace mm6 {

constexpr std::size_t RAW_INVENTORY_ITEM_SLOTS = 138;
constexpr std::size_t RAW_INVENTORY_GRID_CELLS = 126;
constexpr std::size_t RAW_INVENTORY_EQUIPMENT_SLOTS = 16;

// Physical MM6 character item region and its two index tables. The 32-bit
// index values are preserved as bits; this detached owner does not infer which
// of the 138 records are active or reconstruct runtime Item objects.
struct RawCharacterInventory {
    std::array<ItemRecord28, RAW_INVENTORY_ITEM_SLOTS> items = {};
    std::array<std::uint32_t, RAW_INVENTORY_GRID_CELLS> gridWords = {};
    std::array<std::uint32_t, RAW_INVENTORY_EQUIPMENT_SLOTS> equipmentWords = {};

    bool operator==(const RawCharacterInventory &) const = default;
};

struct RawInventoryFields {
    bool ready = false;
    std::array<RawCharacterInventory, 4> characters = {};

    bool operator==(const RawInventoryFields &) const = default;
};

RawInventoryFields captureRawInventory(const RawInventoryFields &owner, GameTitle title);
SaveSection encodeRawInventorySection(const RawInventoryFields &fields);
RawInventoryFields decodeRawInventorySection(const SaveContainer &container);
void loadRawInventoryBeforeMutation(std::span<const std::uint8_t> bytes,
                                    RawInventoryFields *destination, GameTitle title);

} // namespace mm6
