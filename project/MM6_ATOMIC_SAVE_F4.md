# MM6 `.ec6` native file writer and recovery — F4

The standalone `MM6AtomicSaveFile` component provides the disk boundary for a future complete MM6 save transaction. It does not enable save/load menus or call `SaveLoad.cpp`.

## Boundary and sequence

- `writeAtomicSaveFile(profileRoot, slotName, bytes)` takes the **resolved native MM6 user-profile root** (the same path passed to `NativeFileSystem` at startup), an ASCII slot basename ending in `.ec6`, and a finished container. It writes under `profileRoot/saves/`. `profileRoot` must be absolute and trusted; the caller must establish the MM6 title and profile before calling. `ufs->displayPath()` is a display string, not an invertible native path API.
- `decodeSaveContainer()` validates title, version, counts, bounds, section tags and CRCs before directory creation. This is envelope validation only. The future whole-save transaction must also validate all required field families and cross-section state before writing or mutating live game state.
- The writer rejects an existing primary unless it is a valid `.ec6` container. If the primary is missing but a backup exists, it requires recovery first. It creates a unique same-directory temporary file, writes all bytes, syncs the file, closes it, reads and validates it, and compares the readback. If replacing a valid prior save, it writes and syncs a second temp copy of that save, renames that copy to `<slot>.bak`, and syncs the directory. It then renames the new temp to `<slot>` and syncs the directory. A failure before primary rename leaves the original valid primary in place; a failure after replacement may be an ambiguous completed write, which the caller should resolve by reading the slot.
- `loadRecoverableSaveFile()` validates the primary and returns it when sound. If it is missing or invalid, a valid backup is copied through a synced temporary file and atomically restored to the primary; the backup remains intact. When both candidates are absent it returns no value. If neither candidate is valid, or a read/OS operation fails, it throws and does not return bytes to the runtime.
- Temp files are cleaned on exceptions. The writer does not silently delete a corrupt primary or backup. It does not provide cross-process writer locking, save generation counters, or a platform-independent `FileSystem` replacement operation. Concurrent writers to one slot are unsupported.

`FileSystem` and `NativeFileSystem` expose no rename or sync operation. RAM/custom user filesystems and trace replay therefore cannot use this helper as written; the future SaveLoad adapter must branch at the native profile boundary and keep those cases disabled or implement a separate transaction contract. Windows is explicitly unsupported by this POSIX implementation. The MM7 save path remains separate.

## Offline verification

Three focused C++ tests cover initial write and replacement, prior-save backup, corrupt/missing-primary recovery, wrong-title rejection, invalid destination rejection, and injected failures before temp write, after one byte of a temp write, before backup replacement and before primary replacement. They ran standalone with the repository's vendored Google Test sources. No partial MM6 game or original game installation was opened.

## Remaining integration gates

The complete `.ec6` required-field set, cross-section validation, state snapshot ordering, atomic in-memory update, menu slot filter/metadata, thumbnail, and actual map/party reconstruction remain separate work. The writer must only be called after those are ready. Future tests should exercise disk-full and actual OS rename/sync failures on the target volume, as well as app relaunch and a retained MM7 save.
