#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "MM6CoreSaveFields.h"
#include "MM6DurableSaveFields.h"
#include "MM6MapSaveFields.h"
#include "MM6NpcSaveFields.h"
#include "MM6SaveMetadata.h"

namespace mm6 {

// The complete set of codecs currently known to EC6. This is not the complete
// MM6 runtime state and must not be connected to live save/load yet.
struct KnownSaveFields {
    CoreSaveFields core;
    DurableSaveFields durable;
    SaveMetadataFields metadata;
    MapSaveFields maps;
    NpcSaveFields npcs;
};

/** Encode exactly eleven required sections after every field and cross-check. */
std::vector<std::uint8_t> encodeKnownSave(const KnownSaveFields &fields);

/** Reject missing/extra sections and decode all eleven before returning state. */
KnownSaveFields decodeKnownSave(std::span<const std::uint8_t> bytes);

/** Decode and validate the entire known set before assigning the destination. */
void loadKnownSaveBeforeMutation(std::span<const std::uint8_t> bytes, KnownSaveFields *destination);

} // namespace mm6
