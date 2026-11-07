---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, save, recovery]
---

# MM6 F4 Atomic Save File

This page records the bounded native `.ec6` write and recovery seam and its integration limits.

`src/Engine/MM6/MM6AtomicSaveFile.*` decodes all eighteen v1 known sections, or all nineteen v2 raw sections through separate APIs, before any disk change. A valid outer CRC alone cannot promote a partial or malformed save into a primary or backup. The writer uses a synced same-directory temp, preserves a validated old primary in `<slot>.bak`, then atomically renames the new file into place. ASCII `.ec6` basenames and an absolute trusted native MM6 profile root are required. Neither schema can overwrite or recover the other implicitly.

The API is standalone. It does not alter SaveLoad, UI slots, runtime state, MM7 behavior, or abstract `ufs`. RAM/custom `FileSystem` and Windows are unsupported. Both manifests remain incomplete as live transactions: private 67-name map identity, base/delta validation and unresolved runtime fields remain upstream. A native-profile path must come from startup. A failed post-rename directory sync can leave a valid new primary despite an exception; re-read the slot to resolve that case.

The read-only `listKnownSaveSlots` and `listRawSaveSlots` seams supply sorted `.ec6` names, saved `HEAD` metadata and a `needsRecovery` flag for a future MM6 menu. Each checks its own schema in primary or backup, ignores MM7 and temp files, omits structurally invalid slots, and never restores during listing. The directory and candidate files must not be symlinks. Structural listing alone is not proof of a playable campaign; live menu use remains gated by complete state and map-source checks.

The separate v1/v2 `preflightRecoverableSaveFile` and `preflightRecoverableRawSaveFile` seams accept a required semantic validator and select valid primary or backup bytes without restoring either candidate. This allows the eventual MM6 load transaction to check the private map set and base/delta pairs, stage all live state on copies, then decide on recovery and global commit. The corresponding `loadRecoverable*` functions can restore after only structural checks, so neither is a live SaveLoad entry point. The v1 rollback snapshot/restore helper is not a v2 migration path. The [implementation note](../project/MM6_ATOMIC_SAVE_F4.md) records fallback, symlink and failure behavior.

The linked arm64 unit build passed all seven focused atomic-save tests: five retained v1 cases and two v2 cases covering both cross-schema overwrite directions, v2 backup/preflight/recovery, and four injected write failures. No game process or original installation was opened. See [the implementation note](../project/MM6_ATOMIC_SAVE_F4.md) for the exact sequence and open gates.

A later **test-only first-map v5 policy** classifies an atomic attempt from
validated before/after primary and backup bytes. A normal writer return with
the proposed primary and expected backup is committed; a failed call with an
unchanged primary has no new primary; a failed call after primary rename has
an **unconfirmed** new primary. Both failed states require read-only preflight
before further action. The new fault seam injects the last case between
primary rename and directory sync. A title/map/slot-locked v5 preflight
chooses a valid primary or backup without repair, rejects wrong-map and
recognizable foreign-version occupants, and reports `needsRecovery`. Focused
units passed **2/2**, including a disposable post-rename fault and a damaged
primary selecting its intact backup; MM7 control passed **1/1**. No production
save, load, menu listing or rollback was enabled. See the
[first-map v5 packet](../project/MM6_F4_EC6_V5_SPRITE_RUNTIME.md) for the
separate live-owner checks.

See also: [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 Save Spec Source](Source-MM6-Save-Spec.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
