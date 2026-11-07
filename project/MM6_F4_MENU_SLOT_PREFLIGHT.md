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

## Normal UI route and QuickLoad guard

`UISaveLoad.cpp` still returns an empty MM6 load list before its MM7 `.mm7`
decoder. The MM6 Main Menu QuickLoad handler now refuses normal requests
before it probes a possible `quicksaveN.ec6` file or queues a load. Its
explicit test-only `autosave.ec6` override remains available for disposable
fresh-child replay. A focused headless test put a `.ec6` quicksave marker in
GameTest's isolated user RAM filesystem, with an owner-only system-temp
profile as its opt-in gate. The menu stayed in place, no load callback ran or
pending filename appeared, and the marker remained unchanged (**1/1**).
An MM7 headless control passed **1/1**. The native temp profile was removed.

The next UI change needs a separate MM6 `.ec6` semantic slot inventory; the
MM7 `SaveGameLite_MM7` parser, `.mm7` autosave name and free-slot allocator
must stay on the MM7 branch. A selected MM6 slot must pass the complete
read-only load transaction before `LoadSlotState` queues a loading transition,
and a failed publication must not fall through into `Game::gameLoop()`.
The earlier paused-timer capture blocker is superseded for the exact first
map: the read-only v5 engine capture now has a `PausedSaveMenu` boundary that
requires the actual paused Save window and an idle UI queue, then recaptures
the exact live owners. A real-menu headless child passed **1/1**; the unpaused
v5 chain passed **7/7** and MM7 control **1/1**. The disposable writer still
calls the unpaused default, and normal `doSavegame`/UI remains guarded. A
future normal route must select the paused contract, preserve the title/map
and owner-only slot checks, and report success only for a confirmed atomic
result. An unconfirmed post-rename result requires read-only preflight. The
resolved native MM6 profile root must come from startup, including an explicit
user-path override,
not from a display string. Normal SaveLoad and menu listing remain closed.
