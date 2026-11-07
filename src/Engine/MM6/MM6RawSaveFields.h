#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "MM6BuffSaveFields.h"
#include "MM6ChestSaveFields.h"
#include "MM6CoreSaveFields.h"
#include "MM6DurableSaveFields.h"
#include "MM6HirelingSaveFields.h"
#include "MM6MapSaveFields.h"
#include "MM6NpcSaveFields.h"
#include "MM6RawHeldItemSaveFields.h"
#include "MM6RawInventoryFields.h"
#include "MM6RecoverySaveFields.h"
#include "MM6ResistanceSaveFields.h"
#include "MM6SaveMetadata.h"
#include "MM6ShopTitleState.h"

namespace mm6 {

// EC6 v2's coordinated raw-item candidate. It replaces logical ITEM v1 with
// INVR, replaces HELD v1 with raw HELD v2, and requires physical SHOP together
// with its SCLK deadlines. It is still not a complete live MM6 save: beacon,
// active UI and other owner/commit gates remain outside this manifest.
struct RawSaveFields {
    CoreSaveFields core;
    DurableBits bits;
    DurableEffects effects;
    RawInventoryFields inventory;
    SaveMetadataFields metadata;
    MapSaveFields maps;
    NpcSaveFields npcs;
    ResistanceSaveFields resistances;
    RawHeldItemSaveFields heldItem;
    BuffSaveFields buffs;
    HirelingSaveFields hirelings;
    RecoverySaveFields recovery;
    ChestSaveFields chest;
    ShopTitleState shops;
};

/** Encode the exact nineteen-section raw candidate under EC6 container v2. */
std::vector<std::uint8_t> encodeRawSave(const RawSaveFields &fields, GameTitle title);

/** Reject v1, missing/optional/extra sections and bad raw fields before return. */
RawSaveFields decodeRawSave(std::span<const std::uint8_t> bytes, GameTitle title);

/** Assign only after the complete raw candidate has decoded and validated. */
void loadRawSaveBeforeMutation(std::span<const std::uint8_t> bytes,
                               RawSaveFields *destination, GameTitle title);

} // namespace mm6
