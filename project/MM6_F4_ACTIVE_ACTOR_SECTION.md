# MM6 full active-actor EC6 section candidate

Status, 2026-11-07: detached `ACTR` v1 full-list runtime section. It is now
required by the exact v3 manifest but not used by live SaveLoad. Linked ACTR/SPAW unit
suites passed **8/8**, and a fresh-process private-backed headless first-map
ACTR encode/decode/re-encode probe passed **1/1** with ramfs user state and
no profile write. This is capture evidence, not save–quit–reload evidence.

ACTR has a distinct tag and requires actor slots contiguous from zero. It
uses the bounded canonical runtime record codec proved for SPAW, retaining
MM6 resistance bytes, zero descriptor sentinel where valid, blood-splat
flag and mass-distortion timer. A canonical 16-byte zero-list form is
defined. Wrong title, tag, slot, framing, item/buff state and noncanonical
replay refuse. The runtime record codec currently refuses nondefault actor
items or buffs, so later combat/loot states remain outside its admitted
subset. The Apple arm64 snapshot layout must stay pinned for v1; layout
changes require a new version or explicit migration.

ACTR replaces spawned-only SPAW as the candidate active actor
authority because the native DDM writer omits many original-actor changes.
The DDM stays retained as source/opaque map data. The detached v3 manifest
now requires ACTR; map staging must bind it to the active DDM and prove a
nonzero no-respawn day. Live replay must install the full actor list before
generated NPC and map-event readers, then strict post-world identity and
owner checks must pass. No such live route exists yet.
