# MM6 first-map native slot policy

Status, 2026-11-07: detached first-map v5 policy and test-only atomic seam.
Normal `doSavegame`, menu listing, native startup-root handoff and production
profile writes remain guarded.

Update, 2026-11-07: normal `doSavegame` and menu listing now use this policy. The MM6 Save menu writes the selected or first free `saveNNN.ec6` through it and the atomic writer, and startup hands over the native profile root through `mm6::setStartupNativeProfileRoot`. Saving is armed only after a load route admits the session, and today only test code arms it.

`MM6V5FirstMapNativeSlotPolicy` accepts a caller-supplied pair: the native MM6
profile root resolved at startup, including an explicit `--user-path` override,
and the requested root. It requires exact equality, an absolute normalized
canonical directory, and a native non-symlink `saves` directory when present.
It does not infer a root from `ufs->displayPath()`, a home directory, or MM7's
profile. The policy accepts only MM6 title and numbered manual names of the
form `saveNNN.ec6`; quicksave, autosave, candidate and `.mm7` names refuse.

The read-only preflight validates the proposed complete v5 container, first
map `HEAD`, and required semantic callback. It snapshots bounded primary and
backup bytes. Any existing primary or backup must independently pass the same
first-map v5 and semantic checks. A backup without a primary, a damaged file,
a recognizable older/newer version, another map, a non-regular file, or a
symlinked saves path refuses before writing. This conservatively prevents a
new manual save from replacing an ambiguous or foreign slot.

The separate `writeV5FirstMapNativeCandidateForTest` seam then uses a
numbered-slot atomic v5 writer. A normal return with exact primary and backup
bytes is `Committed`. An injected fault before primary replacement, leaving
the prior primary unchanged, is `NoNewPrimaryNeedsPreflight`. A fault after
primary rename remains `IndeterminateNeedsPreflight` even when the new bytes
are readable. Any unexpected post-attempt state is also indeterminate. No
non-committed outcome can close a Save menu or claim durability.

Two focused synthetic units passed **2/2** after integration. The first
covered title/root/slot refusal and foreign, other-map, corrupt, backup-only,
corrupt-backup and symlink collisions without changing files. The second
verified exact primary/backup bytes and all three outcome classes under
disposable native atomic writes. MM7 Prs passed **1/1**. Earlier headless work
proves a paused real Save-menu candidate and a distinct disposable writer plus
fresh-child replay; this unit policy has not yet written a real paused
candidate under a numbered native slot.

The low-level native atomic entry remains test-only and does not itself prove
root authority. Before normal wiring, startup must pass its resolved MM6 root
to the save transaction as an owned value, then the Save menu must allocate
or select an admitted numbered slot and route only a confirmed result to UI
success. Cross-process slot races and recovery after an indeterminate write
are not yet production-admitted. The existing owner-only candidate writer and
its two-slot restriction are unchanged.

See [v5 first-map runtime](MM6_F4_EC6_V5_SPRITE_RUNTIME.md) and
[menu slot preflight](MM6_F4_MENU_SLOT_PREFLIGHT.md).
