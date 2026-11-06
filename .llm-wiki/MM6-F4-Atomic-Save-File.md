---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, save, recovery]
---

# MM6 F4 Atomic Save File

This page records the bounded native `.ec6` write and recovery seam and its integration limits.

`src/Engine/MM6/MM6AtomicSaveFile.*` now decodes all sixteen currently known required sections before any disk change. A valid outer CRC alone cannot promote a partial or malformed save into a primary or backup. The writer uses a synced same-directory temp, preserves a validated old primary in `<slot>.bak`, then atomically renames the new file into place. The loader applies the same known-field checks before returning a primary or restoring a backup. ASCII `.ec6` basenames and an absolute trusted native MM6 profile root are required.

The API is standalone. It does not alter SaveLoad, UI slots, runtime state, MM7 behavior, or abstract `ufs`. RAM/custom `FileSystem` and Windows are unsupported. The sixteen-section manifest is partial: private 67-name map identity, base/delta validation and unresolved runtime fields remain upstream of a live save transaction. A native-profile path must come from startup. A failed post-rename directory sync can leave a valid new primary despite an exception; re-read the slot to resolve that case.

The read-only `listKnownSaveSlots` seam supplies sorted `.ec6` names, saved `HEAD` metadata and a `needsRecovery` flag for a future MM6 menu. It checks all known sections in each primary or backup. It ignores MM7 and temp files, omits slots without a valid candidate, and does not restore a backup during listing. The directory and candidate files must not be symlinks. Live menu use remains gated by the incomplete `.ec6` state and map-source checks.

The arm64 MM6/MM7/unit build passed. Four focused tests passed across initial/replacement, CRC-valid malformed or missing primary recovery, rejection of partial and malformed candidates, injected partial write/backup/primary replacement faults, and read-only slot filtering including symlink rejection. No game process or original installation was opened. See [the implementation note](../project/MM6_ATOMIC_SAVE_F4.md) for the exact sequence and open gates.

See also: [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 Save Spec Source](Source-MM6-Save-Spec.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
