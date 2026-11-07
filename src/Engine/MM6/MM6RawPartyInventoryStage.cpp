#include "MM6RawPartyInventoryStage.h"

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "Engine/Engine.h"

#include "MM6BoundedPartyInventoryProjection.h"

namespace mm6 {
namespace {

void requireMM6(const ItemTable &table, GameTitle title) {
    if (title != GameTitle::MM6 || pItemTable != &table ||
        (engine && engine->gameTitle() != GameTitle::MM6))
        throw std::runtime_error("MM6 raw inventory stage requires a title-local item table");
}

} // namespace

StagedRawPartyInventories stageRawPartyInventories(
    const Party &baseline, const RawInventoryFields &raw,
    const ItemTable &table, GameTitle title) {
    requireMM6(table, title);
    auto projected = stageBoundedPartyInventory(raw, table, title);
    if (!projected)
        throw std::runtime_error("MM6 raw inventory has unsupported item semantics or placement");

    Party party = baseline;
    for (std::size_t index = 0; index < party.pCharacters.size(); ++index)
        party.pCharacters[index].inventory = projected->inventory(index);
    return {std::move(party), raw};
}

RawInventoryFields captureRawPartyInventories(
    const StagedRawPartyInventories &staged,
    const ItemTable &table, GameTitle title) {
    requireMM6(table, title);
    auto projected = stageBoundedPartyInventory(staged.raw, table, title);
    if (!projected)
        throw std::runtime_error("MM6 raw inventory source is no longer projectable");
    for (std::size_t index = 0; index < staged.party.pCharacters.size(); ++index)
        projected->inventory(index) = staged.party.pCharacters[index].inventory;
    auto captured = captureBoundedPartyInventory(*projected, table, title);
    if (!captured)
        throw std::runtime_error("MM6 raw inventory edits are not reversible");
    return std::move(*captured);
}

} // namespace mm6
