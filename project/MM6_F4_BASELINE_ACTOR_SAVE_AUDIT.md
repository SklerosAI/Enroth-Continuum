# MM6 baseline actor save boundary

Status, 2026-11-07: source audit plus disposable headless full-list probe.
The detached spawned-only SPAW v1 and exact EC6 v3 manifest are **not** a
durable post-entry actor save design yet. A fresh-process first-map probe
that attempted to encode the full active actor list initially refused with
`missing-mm6-resistance`. After exact native resistance restoration, the
same probe advanced and refused with `identity` (**0/1**); the
appended-spawn-only capture passed **1/1**. The probe used ramfs user state
and wrote no profile.

`encodeMM6OutdoorChangedState()` currently requires live actor count equal
to the native DDM actor count, so it cannot write a map after encounter
spawning. If that count is reconciled, it copies only attributes, HP and
selected snapshot ranges for each original actor into the retained raw
sidecar. Other runtime fields are silently omitted because the candidate
begins with original DDM bytes. The omitted areas include generated NPC
links, some monster/hostility state, and native buff/item and later actor
regions. `OutdoorLocation::Load` also normalizes actors during first entry.
The MM6 loader now extracts all six resistance bytes from each retained
native actor record and assigns the exact tuple after reconstruction. The
focused helper test passed **1/1** with differing per-actor values; MM7's
route is unchanged. The field offsets match the pinned public
[MM6 editor record layout](https://github.com/angeld29/mm8leveleditor/blob/bf3b8c4a86100635a5fc6ff53fac6e4e0af5cb53/src/org/gamenet/application/mm8leveleditor/data/mm6/Creature.java#L83-L88).
The remaining `identity` refusal is under source audit; no monster or slot
check has been relaxed.

Before any v3 disk or live route, prove an exact baseline actor owner or a
strict replay-equivalence gate that refuses every unsupported runtime
mutation. A full active-actor EC6 section is a candidate, but it needs
exact native resistance provenance and a round-trip for mutated actors,
including item/buff state. A spawned-only section can remain a narrow
first-map capture but cannot support durable campaign play by itself. Keep
strict PID admission and the expected bootstrap refusal.
