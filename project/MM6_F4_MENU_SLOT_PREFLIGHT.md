# MM6 v2 menu slot preflight

Status, 2026-11-07: detached native listing seam only. The visible MM6 load
menu remains empty; normal MM6 `loadGame()` and Atlas Play remain guarded.

`listRawSaveSlots()` proves only the EC6 v2 container and field syntax. It can
show a slot whose inventory, map or runtime owners fail later staging. The new
`listRawSaveSlotsWithPreflight(profile, validate)` enumerates only those
structural candidates, then calls the same read-only primary/backup selector
used by v2 load preflight for each slot. It takes a required semantic validator
and displays the HEAD from the *selected* candidate, including its
`needsRecovery` flag. A valid backup can therefore supply the menu header when
the primary is structurally valid but fails semantic validation. A slot with
no accepted candidate is omitted. The operation never repairs or writes disk;
symlink and I/O errors still propagate rather than turning into an entry.

The synthetic atomic-file test covers rejected primary/accepted backup,
another structurally valid but semantically rejected slot, null validator,
the selected display name and unchanged primary/backup bytes. It does not
prove any real slot playable: a partial validator can accept a partial v2
session. The caller must provide the final complete MM6 load admission
validator before `GUIWindow_Load` uses this list. Current `UISaveLoad.cpp`
returns no MM6 entries, and `LoadSlotState` must not queue an MM6 filename
until that same validator and production `loadGame()` branch are ready.

The native profile root remains separate from the MM7 `ufs` saves folder.
Original game data and saves remain outside Git.
