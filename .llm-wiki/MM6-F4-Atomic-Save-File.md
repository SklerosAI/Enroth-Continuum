---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, save, recovery]
---

# MM6 F4 Atomic Save File

This page records the bounded native `.ec6` write and recovery seam and its integration limits.

`src/Engine/MM6/MM6AtomicSaveFile.*` validates an MM6 container before any disk change, writes a synced same-directory temp, preserves a validated old primary in `<slot>.bak`, then atomically renames the new file into place. The loader validates the primary, restores a valid backup through another temp when necessary, and rejects a slot with no valid copy. ASCII `.ec6` basenames and an absolute trusted native MM6 profile root are required.

The API is standalone. It does not alter SaveLoad, UI slots, runtime state, MM7 behavior, or abstract `ufs`. RAM/custom `FileSystem` and Windows are unsupported. The parent transaction still needs complete field and cross-section validation and a native-profile path supplied by startup. A failed post-rename directory sync can leave a valid new primary despite an exception; re-read the slot to resolve that case.

Standalone C++ tests passed initial/replacement, corrupt or missing primary recovery, malformed candidate rejection, and injected partial write/backup/primary replacement faults while the previous valid save remained accessible. No game process or original installation was opened. See [the implementation note](../project/MM6_ATOMIC_SAVE_F4.md) for the exact sequence and open gates.

See also: [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 Save Spec Source](Source-MM6-Save-Spec.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
