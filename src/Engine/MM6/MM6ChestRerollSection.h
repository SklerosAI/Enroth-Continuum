#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>

#include "Engine/GameTitle.h"

#include "MM6SaveContainer.h"

namespace mm6 {

/**
 * One chest's item slots as they were before its random loot was first rolled: the random-loot marker -1 to -7, 0 for
 * an empty slot and 1 for a fixed item.
 */
using ChestRerollLayout = std::array<std::int8_t, 140>;

/**
 * The chests that the chest reroll mod can still reroll, by canonical map file name and chest index.
 */
using ChestRerollLayouts = std::map<std::pair<std::string, int>, ChestRerollLayout>;

/**
 * @param layouts                       Rerollable chests.
 * @param title                         Game title, only MM6 is accepted.
 * @return                              Optional `CHRL` section, or `std::nullopt` when no chest can be rerolled, which a
 *                                      save leaves out. Throws for a layout the section can't hold.
 */
std::optional<SaveSection> encodeChestRerollSection(const ChestRerollLayouts &layouts, GameTitle title);

/**
 * @param container                     Container with at most one `CHRL` section.
 * @param title                         Game title, only MM6 is accepted.
 * @return                              Decoded layouts, empty for a save without the section. Throws on a malformed
 *                                      section.
 */
ChestRerollLayouts decodeChestRerollSection(const SaveContainer &container, GameTitle title);

} // namespace mm6
