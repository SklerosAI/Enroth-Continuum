---
type: synthesis
up: "[[Agent-Handoff]]"
tags: [mm6, f4, transport, private-data]
---

# MM6 F4 Transport Manifest

The [F4 transport packet](../project/MM6_F4_TRANSPORT_MANIFEST.md) describes a private `rules/transport.json` exported from the corrected F3 PE v2 static analysis. It holds 36 route descriptors and the three slots for each of 21 transport houses. The exporter verifies the ignored F3 report against the gate hash registry and writes mode-0600 inside the MM6 profile. The route constants themselves remain outside Git.

Do not borrow MM7's static transport schedule or treat a blank `2devents.txt` cell as a route. Thirty-four cells have regular schedule candidates, one has an unresolved weekday token and two are opaque specials. The manifest preserves those categories. The MM6 menu now consumes it, checks the original one-based Games LOD destination index, and renders three scheduled choices. It hides uncertain/special slots and nonzero quest-bit candidates. Selection returns without gold, travel or autosave while the `.ec6` transaction is incomplete. Fare, quest gating, the indoor destination, specials and save behavior still need F4/F5 proof.

For parallel work, read the [F3 gate](MM6-F3-Read-Only-Preflight.md), [F4 transport packet](../project/MM6_F4_TRANSPORT_MANIFEST.md), [source privacy boundary](Source-and-Privacy-Boundary.md) and [current F4 seams](MM6-F4-Offline-Seams.md). No partial MM6 game was launched for this export.
