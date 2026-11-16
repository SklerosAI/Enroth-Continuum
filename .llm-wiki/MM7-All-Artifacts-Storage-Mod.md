---
type: project
up: "[[Home_Enroth-Continuum]]"
tags: [mm7, mod, artifacts, storage, save]
---

# MM7 All Artifacts + Storage

The [implementation packet](../project/MM7_ALL_ARTIFACTS_STORAGE_MOD.md) defines
the optional MM7-only completionist rule. The Atlas passes exact
`ENROTH_MM7_ALL_ARTIFACTS_STORAGE=1` only for an enabled Private mods launch;
the source default and other titles remain off.

Random generation continues to use the 29 IDs in `allSpawnableArtifacts()`.
The native generated ledger remains intact, while a separate monotonic collected
ledger controls eligibility when the mod is active. Actual cursor, party-add and
direct chest-grab acquisition marks collection. The versioned ledger occupies
reserved bytes in the fixed-size MM7 party snapshot and survives later disabled
launches. Legacy migration scans backpacks, equipment and cursor only; older
stored/world artifacts become collected on their next withdrawal or pickup.

Castle Harmondale's twenty existing `d29` chests remain the storage system.
Their full item records and grid are map-delta state, the castle is excluded
from timed indoor respawn, and renovation does not replace the delta. No new
vault UI or map-reset exception was required. Representative engine and storage
tests cover all 29 collected IDs, both generation routes, exhaustion, migration,
the exact chest-event permutation and one full-field chest roundtrip.

Simultaneous uncollected world copies remain possible; future generation stops
after the first real acquisition. MM6 work is a Claude-owned future plan, not
part of this implementation.

