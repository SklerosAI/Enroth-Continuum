# MM6 `.ec6` native file writer and recovery — F4

The standalone `MM6AtomicSaveFile` component provides the disk boundary for a future complete MM6 save transaction. It does not enable save/load menus or call `SaveLoad.cpp`.

## Boundary and sequence

- `writeAtomicSaveFile(profileRoot, slotName, bytes)` takes the **resolved native MM6 user-profile root** (the same path passed to `NativeFileSystem` at startup), an ASCII slot basename ending in `.ec6`, and a finished container. It writes under `profileRoot/saves/`. `profileRoot` must be absolute and trusted; the caller must establish the MM6 title and profile before calling. `ufs->displayPath()` is a display string, not an invertible native path API.
- The provisional v1 `decodeKnownSave()` validates eighteen required sections, field codecs and cross-field clocks, selection and active-map companion before directory creation. A CRC-valid partial container or a malformed field cannot become a primary or backup. The future whole-save transaction must also validate the private 67-name map set, every base/delta pair and the still-missing runtime state before writing or mutating live game state.
- The writer rejects an existing primary unless it passes the same known-save validation. If the primary is missing but a backup exists, it requires recovery first. It creates a unique same-directory temporary file, writes all bytes, syncs the file, closes it, reads and validates it, and compares the readback. If replacing a valid prior save, it writes and syncs a second temp copy of that save, renames that copy to `<slot>.bak`, and syncs the directory. A failure before primary rename leaves the original valid primary in place; a failure after replacement may be an ambiguous completed write, which the caller should resolve by reading the slot.
- `loadRecoverableSaveFile()` validates all known sections in the primary and returns it when sound. If it is missing or invalid, a backup passing the same checks is copied through a synced temporary file and atomically restored to the primary; the backup remains intact. When both candidates are absent it returns no value. If neither candidate is valid, or a read/OS operation fails, it throws and does not return bytes to the runtime.
- Temp files are cleaned on exceptions. The writer does not silently delete a corrupt primary or backup. It does not provide cross-process writer locking, save generation counters, or a platform-independent `FileSystem` replacement operation. Concurrent writers to one slot are unsupported.

`FileSystem` and `NativeFileSystem` expose no rename or sync operation. RAM/custom user filesystems and trace replay therefore cannot use this helper as written; the future SaveLoad adapter must branch at the native profile boundary and keep those cases disabled or implement a separate transaction contract. Windows is explicitly unsupported by this POSIX implementation. The MM7 save path remains separate.

## Read-only MM6 slot inventory

`listKnownSaveSlots(profileRoot)` scans only the native MM6 profile's `saves` directory. It accepts ASCII `.ec6` basenames and `.ec6.bak` companions, ignores `.mm7`, temporary and unrelated files, and returns sorted filename, saved `HEAD` metadata and a `needsRecovery` flag. Each candidate must decode all eighteen known sections. A valid primary wins; a backup appears only when the primary is absent or has invalid known fields. A malformed slot with no valid candidate is omitted. The listing never restores a backup or changes files; a recovery-needed entry must go through `loadRecoverableSaveFile` before a future load. A symlinked save directory or candidate file is rejected rather than traversed.

This is a menu source seam, not a menu integration or a complete save guarantee. The current known-field decoder does not verify the private 67-name map set or every base/delta pair, and unresolved runtime state is outside its eighteen sections. The MM6 SaveLoad guard stays closed.

## Read-only load preflight

`preflightRecoverableSaveFile(profileRoot, slotName, validate)` selects a primary or backup without copying, renaming or restoring either file. Every candidate first passes all eighteen known-section checks, then a required caller-supplied semantic validator. A valid primary wins; a failed primary can fall back to a valid backup. The result contains detached candidate bytes and a `needsRecovery` flag. If both existing candidates fail validation, it throws with the files untouched; a missing slot returns no value. Symlinked save directories or candidate files reject before data is read. The validator signals an invalid candidate with `std::runtime_error`; an I/O error still propagates.

This is the disk-read half of a future MM6 SaveLoad adapter. The caller must pass a validator that checks the private 67-name set, every base/delta pair, loaded item templates and eventually all remaining runtime fields. After preflight it must stage the complete party/world/NPC/timer state on copies before deciding whether to restore the backup and commit globals. The existing `loadRecoverableSaveFile` restores after the partial known-field check and is **not** sufficient for that live path. No menu or SaveLoad call uses the new preflight yet.

## Separate v2 raw slot boundary

`writeAtomicRawSaveFile`, `listRawSaveSlots`, `preflightRecoverableRawSaveFile` and `loadRecoverableRawSaveFile` use the same native disk protocol with the nineteen-section EC6 v2 raw decoder. A v1 file cannot be overwritten or recovered as v2, and a v2 file cannot be overwritten or recovered as v1. There is no implicit migration. The v1 rollback snapshot/restore test helper remains v1-only.

The v2 list and recovery helpers establish structural integrity, not campaign playability. A future live loader must use read-only v2 preflight with full map/base and owner staging before any backup repair or live publication. The v2 candidate is still detached; none of these APIs is called from normal MM6 SaveLoad or the load menu.

## First-map v5 result and read-only recovery policy

The existing disposable v5 writer now has a fault-injection point immediately
after primary rename and before directory sync. A failed call at that point
can leave the proposed bytes as a readable primary without proving the
directory update durable. The new read-only
`assessV5FirstMapWriteResultForTest` compares validated primary/backup bytes
before and after an attempt. It reports `Committed` only if the writer returned
normally and the exact proposed primary and expected prior backup are present.
After an exception, an unchanged primary is `NoNewPrimaryNeedsPreflight`; a
new proposed primary is `NewPrimaryUnconfirmedNeedsPreflight`. Neither failed
outcome authorizes a success message, automatic retry, or gameplay admission.
Unexpected changes or invalid candidates refuse.

The policy accepts only MM6, the first outdoor map in the v5 `HEAD`, and the
two existing first-map v5 candidate slot names. Its required semantic callback
must check the full source and owner graph; a decoder-only callback is enough
for the synthetic unit fixture but not for a future live load. The matching
`preflightRecoverableV5FirstMapSaveFileForTest` selects primary or backup
read-only, returns whether recovery would be needed, and rejects a recognizable
foreign EC6 version or a valid v5 occupant for another map. A damaged primary
may fall back to a valid first-map backup, leaving both disk files untouched.
The result policy and preflight are test-only; normal SaveLoad and UI remain
closed, and no production slot or rollback procedure has been admitted.

Two focused linked units passed **2/2** after integration. They exercise
the three result states and wrong-title/map/slot/backup refusals, plus an
owner-only disposable post-rename fault, read-only selection of the new
primary, and backup selection after deliberate primary corruption without
disk repair. An MM7 headless control passed **1/1**. The staged first-map
v5 live capture and post-world owner validator remain separate prerequisites;
the [v5 packet](MM6_F4_EC6_V5_SPRITE_RUNTIME.md) records their seven-child
game-loop evidence.

## Offline verification

The linked arm64 unit target built and all seven focused atomic-save tests passed. The original five cover v1 initial write/replacement, backup recovery, malformed candidates, invalid destination, fault injection, slot listing and semantic preflight. The two v2 tests cover both directions of cross-schema overwrite rejection, v2 backup and read-only preflight, structural recovery, and four injected write failures that preserve the prior slot. No game process or original game installation was opened for these tests.

## Remaining integration gates

Both schemas remain incomplete as live transactions. Verified 67-name map identity/base relations, unresolved runtime state, snapshot ordering, atomic in-memory update, menu slot filter/metadata, thumbnail, and actual map/party reconstruction remain separate work. The v2 writer must only be called after those are ready. Future tests should exercise disk-full and actual OS rename/sync failures on the target volume, as well as app relaunch and a retained MM7 save.
