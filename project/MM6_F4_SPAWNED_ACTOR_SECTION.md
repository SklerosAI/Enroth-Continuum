# MM6 spawned-actor EC6 section candidate

Status, 2026-11-07: detached `SPAW` v1 runtime section, not in the strict
EC6 v2 manifest or live SaveLoad. The linked unit suite passed **4/4** and a
private-backed, fresh-process headless first-map encode/decode/re-encode
capture passed **1/1**. The probe used ramfs user state and did not write a
profile or game save. It does not prove save–quit–reload fidelity.

The section records an exact first PID slot and a contiguous actor count,
then a canonical current-runtime `Actor_MM7` shaped snapshot for each actor
plus an MM6-specific six-byte resistance tuple, blood-splat flag and
mass-distortion timer. This is an EC6 runtime representation, not a fabricated
native 548-byte DDM record. An exact 16-byte zero-row form retains the
baseline slot for maps with no new actors. The codec validates the MM6 title,
bounded slot/count/size, monster identity, canonical wire framing and
round-trip representation. It refuses unsupported item or buff contents,
missing MM6 resistances, narrowing of positions/timers/hostility, and
noncanonical replay. This initial admissible subset covers the sampled
first-map pristine spawns; later loot or combat states still need an exact
MM6 owner before they can be saved.

The wire is currently tied to the pinned Apple arm64 runtime snapshot layout;
any later layout change needs a new section version or explicit migration.
The headless pass proves only a detached capture from one loaded world. The
next durable step is to join `SPAW` with a strict new EC6 manifest, prove the
baseline DDM actor count and nonzero respawn day, reconstruct at exact slots
before generated NPC replay and map events, then save in a disposable profile
and load in a fresh child. Strict post-world PID and other owner checks must
still pass before first input. The first-map bootstrap v2 refusal remains
correct and should not be weakened.
