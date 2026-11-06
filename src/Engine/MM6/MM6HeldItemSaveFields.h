#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "Engine/GameTitle.h"

#include "MM6SaveContainer.h"

struct Party;
struct ItemTable;

namespace mm6 {

// Item scalars stored without MM7 item-category dispatch. The cursor bitmap
// and click offset are UI state, reconstructed only after a full load commits.
struct HeldItemSaveFields {
    bool present = false;
    std::int32_t itemId = 0;
    std::int32_t potionPower = 0;
    std::int32_t goldAmount = 0;
    std::int32_t standardEnchantment = -1;
    std::int32_t standardStrength = 0;
    std::uint32_t specialEnchantment = 0;
    std::int32_t charges = 0;
    std::int32_t maxCharges = 0;
    std::uint32_t flags = 0;
    std::int32_t lichJarCharacterIndex = -1;
    std::int64_t expirationTicks = 0;

    bool operator==(const HeldItemSaveFields &) const = default;
};

/** Encode one required, versioned HELD section after bounded validation. */
SaveSection encodeHeldItemSection(const HeldItemSaveFields &fields);
HeldItemSaveFields decodeHeldItemSection(const SaveContainer &container);
void loadHeldItemBeforeMutation(std::span<const std::uint8_t> bytes,
                                HeldItemSaveFields *destination);

/** Capture from an active MM6 party; an empty cursor becomes canonical empty fields. */
HeldItemSaveFields captureHeldItem(const Party &party, GameTitle title);

/** Validate the item against loaded MM6 tables and assign only to a party copy. */
Party stageHeldItemParty(const Party &live, std::span<const std::uint8_t> bytes, GameTitle title);

struct PendingActionStateForSave {
    bool spellQueueActive = false;
    bool targetedSpellWindow = false;
    bool enchanting = false;
    bool turnModeActive = false;
    bool turnQueueActive = false;
    bool turnActionsPending = false;
    bool turnPendingFlag = false;
    bool timerTurnModeActive = false;
    bool restActionActive = false;
    bool restWindowActive = false;
    bool guiMessageQueued = false;
    bool movementActionQueued = false;
    bool delayedEnchantMessage = false;
    bool attackListActive = false;
    bool playerSelectionActive = false;
};

struct HeldCursorRestorePlan {
    std::string iconName = "MICON1";
    bool itemCursor = false;
    // The held-item click offset is transient and resets after load.
    bool resetClickOffset = true;
    bool operator==(const HeldCursorRestorePlan &) const = default;
};

// Read-only live action probe. Active spells/targeting/turn work are rejected
// by the detached cursor plan rather than silently dropped on save/reload.
PendingActionStateForSave capturePendingActionStateForSave(GameTitle title);

// Validate a saved HELD field against explicit loaded MM6 templates and
// stage only the post-commit cursor intent. No mouse, image or UI is touched.
HeldCursorRestorePlan stageHeldCursorRestorePlan(
    const HeldItemSaveFields &held, const ItemTable &items,
    const PendingActionStateForSave &actions, GameTitle title);

} // namespace mm6
