# MM6 baseline actor save boundary

Status, 2026-11-07: source audit plus disposable headless full-list probe.
The detached spawned-only SPAW v1 and exact EC6 v3 manifest are **not** a
durable post-entry actor save design yet. A fresh-process first-map probe
that attempted to encode the full active actor list with the current runtime
codec refused with the public class `missing-mm6-resistance` (expected,
**0/1**); the appended-spawn-only capture still passed **1/1**. The probe
used ramfs user state and wrote no profile.

`encodeMM6OutdoorChangedState()` currently requires live actor count equal
to the native DDM actor count, so it cannot write a map after encounter
spawning. If that count is reconciled, it copies only attributes, HP and
selected snapshot ranges for each original actor into the retained raw
sidecar. Other runtime fields are silently omitted because the candidate
begins with original DDM bytes. The omitted areas include generated NPC
links, some monster/hostility state, and native buff/item and later actor
regions. `OutdoorLocation::Load` also normalizes actors during first entry.
The current MM6 projection does not populate the runtime `mm6Resistances`
optional on every baseline actor, which is why the full-list SPAW probe
refused. No default resistance should be invented to satisfy this check.

Before any v3 disk or live route, prove an exact baseline actor owner or a
strict replay-equivalence gate that refuses every unsupported runtime
mutation. A full active-actor EC6 section is a candidate, but it needs
exact native resistance provenance and a round-trip for mutated actors,
including item/buff state. A spawned-only section can remain a narrow
first-map capture but cannot support durable campaign play by itself. Keep
strict PID admission and the expected bootstrap refusal.
