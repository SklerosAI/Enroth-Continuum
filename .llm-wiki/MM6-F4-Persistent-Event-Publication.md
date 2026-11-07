---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Raw-Save-Runtime]]"]
tags: [mm6, f4, save, events]
---

# MM6 F4 Persistent Event Publication

The [implementation packet](../project/MM6_F4_PERSISTENT_EVENT_PUBLICATION.md) adds a test-only fresh-child handoff after v2 detached event staging. `publishFreshChildPersistentEventZeros` validates the stage and assigns only zero inherited 75/125 generic arrays. The proved MM6 event bytes remain in the selected DLV/DDM sidecar. `requirePostMapPersistentEvents` later checks that the reloaded sidecar's 200-byte persistent section still equals the staged 71 bytes plus 129 zero bytes and that both generic arrays remain zero. Two linked synthetic tests passed; neither function opens SaveLoad or the Atlas MM6 Play route.

The reload sequence runs `onMapLoad()` after reconstruction, so a legitimate event write can make this gate refuse the slot. The split and meaning of the upper 129 bytes, decoration state, event replay, trigger lists and whole-session effects remain unproved. Active VISU remains blocked. The v2 fresh-child coordinator must run the post-map check against the actual sidecar before enabling controls; detached preflight alone is insufficient.

See also: [Raw Save Runtime](MM6-F4-Raw-Save-Runtime.md), [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), and the [detached persistent-event boundary packet](../project/MM6_F4_PERSISTENT_EVENT_BOUNDARY.md).
