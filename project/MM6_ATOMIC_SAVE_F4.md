# MM6 `.ec6` native file writer and recovery — F4

The standalone `MM6AtomicSaveFile` component provides the disk boundary for a future complete MM6 save transaction. It does not enable save/load menus or call `SaveLoad.cpp`.

## Boundary and sequence

- `writeAtomicSaveFile(profileRoot, slotName, bytes)` takes the **resolved native MM6 user-profile root** (the same path passed to `NativeFileSystem` at startup), an ASCII slot basename ending in `.ec6`, and a finished container. It writes under `profileRoot/saves/`. `profileRoot` must be absolute and trusted; the caller must establish the MM6 title and profile before calling. `ufs->displayPath()` is a display string, not an invertible native path API.
- `decodeKnownSave()` validates the current sixteen required sections, field codecs and cross-field clocks, selection and active-map companion before directory creation. A CRC-valid partial container or a malformed field cannot become a primary or backup. The future whole-save transaction must also validate the private 67-name map set, every base/delta pair and the still-missing runtime state before writing or mutating live game state.
- The writer rejects an existing primary unless it passes the same known-save validation. If the primary is missing but a backup exists, it requires recovery first. It creates a unique same-directory temporary file, writes all bytes, syncs the file, closes it, reads and validates it, and compares the readback. If replacing a valid prior save, it writes and syncs a second temp copy of that save, renames that copy to `<slot>.bak`, and syncs the directory. A failure before primary rename leaves the original valid primary in place; a failure after replacement may be an ambiguous completed write, which the caller should resolve by reading the slot.
- `loadRecoverableSaveFile()` validates all known sections in the primary and returns it when sound. If it is missing or invalid, a backup passing the same checks is copied through a synced temporary file and atomically restored to the primary; the backup remains intact. When both candidates are absent it returns no value. If neither candidate is valid, or a read/OS operation fails, it throws and does not return bytes to the runtime.
- Temp files are cleaned on exceptions. The writer does not silently delete a corrupt primary or backup. It does not provide cross-process writer locking, save generation counters, or a platform-independent `FileSystem` replacement operation. Concurrent writers to one slot are unsupported.

`FileSystem` and `NativeFileSystem` expose no rename or sync operation. RAM/custom user filesystems and trace replay therefore cannot use this helper as written; the future SaveLoad adapter must branch at the native profile boundary and keep those cases disabled or implement a separate transaction contract. Windows is explicitly unsupported by this POSIX implementation. The MM7 save path remains separate.

## Read-only MM6 slot inventory

`listKnownSaveSlots(profileRoot)` scans only the native MM6 profile's `saves` directory. It accepts ASCII `.ec6` basenames and `.ec6.bak` companions, ignores `.mm7`, temporary and unrelated files, and returns sorted filename, saved `HEAD` metadata and a `needsRecovery` flag. Each candidate must decode all sixteen known sections. A valid primary wins; a backup appears only when the primary is absent or has invalid known fields. A malformed slot with no valid candidate is omitted. The listing never restores a backup or changes files; a recovery-needed entry must go through `loadRecoverableSaveFile` before a future load. A symlinked save directory or candidate file is rejected rather than traversed.

This is a menu source seam, not a menu integration or a complete save guarantee. The current known-field decoder does not verify the private 67-name map set or every base/delta pair, and unresolved runtime state is outside its sixteen sections. The MM6 SaveLoad guard stays closed.

## Offline verification

The arm64 MM6/MM7/unit targets built and all four focused atomic-save tests passed. They cover initial write and replacement, prior-save backup, CRC-valid malformed-section and missing-primary recovery, rejection of wrong-title, partial and malformed known-save candidates, invalid destination rejection, and injected failures before temp write, after one byte of a temp write, before backup replacement and before primary replacement. The slot-listing test checks valid primary, backup-only, malformed-primary/valid-backup, malformed-only, symlink and wrong-title/temporary cases without modifying any candidate. No partial MM6 game or original game installation was opened.

## Remaining integration gates

The sixteen-section manifest is still a partial candidate. Verified 67-name map identity/base relations, unresolved runtime state, snapshot ordering, atomic in-memory update, menu slot filter/metadata, thumbnail, and actual map/party reconstruction remain separate work. The writer must only be called after those are ready. Future tests should exercise disk-full and actual OS rename/sync failures on the target volume, as well as app relaunch and a retained MM7 save.
