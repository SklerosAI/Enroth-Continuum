---
type: implementation
up: "[[MM6-F4-Offline-Seams]]"
tags: [mm6, f4, rooms, media, house]
---

# MM6 F4 Room Descriptors

The [room source packet](../project/MM6_F4_ROOM_DESCRIPTORS.md) and [private companion](Private-Companion-Wiki.md) describe an owner-only 119-entry manifest generated from the accepted corrected F3 PE report. The decoder validates the private manifest offline and checks every retained house room reference. All 674 active house-entry instructions are classified: 671 numeric room references, one blank room field and two special terminal IDs.

The MM6 house entry guard remains closed. The descriptor is not wired to live house media, NPCs, sounds or services; the blank and terminal paths still need exact behavior. Future agents should start from the F3 gate hash registry, the ignored format-2 house-route report and the owner-only `house-rooms.json`, then verify field consumers before narrowing the entry gate.
