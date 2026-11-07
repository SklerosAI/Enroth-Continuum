#pragma once

#include <vector>

#include "Engine/GameTitle.h"

#include "MM6RawCursorImagePlan.h"

struct ItemTable;

namespace mm6 {
struct StagedRawSessionFields;
}

namespace mm6::testing {

enum class RawChildPublicationBlocker {
    BackupNeedsRecovery,
    HeldItemOffsetUnresolved,
    MouseAssetNotResolved,
    NpcStaticResourcesNotBound,
    ActiveWorldResourcesNotBound,
    ChestRuntimeCollectionNotJoined,
    RebuiltObjectIdentityUnchecked,
    PersistentEventsNotCheckedAfterWorldLoad,
    BeaconAndIndoorFaceOwnersNotPublished,
    EventTimersAndTriggersNotReset,
    TransientUiAndActionsNotReset,
    PostWorldAdmissionHookMissing,
};

struct RawChildPublicationAdmission {
    RawCursorImagePlan cursorImage;
    std::vector<RawChildPublicationBlocker> blockers;

    bool canEnterGameLoop() const { return blockers.empty(); }
};

// Recheck represented v2 owners after staging and before any fresh-child
// assignment. Returns typed missing destination proofs. This test-only API
// never mutates globals, files, renderer resources or the supplied session.
RawChildPublicationAdmission inspectRawChildPublication(
    const StagedRawSessionFields &staged, const ItemTable &itemTable, GameTitle title);

} // namespace mm6::testing
