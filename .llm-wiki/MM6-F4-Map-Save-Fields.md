---
type: synthesis
up: "[[Agent-Handoff]]"
related: ["[[MM6-F4-Offline-Seams]]", "[[MM6-F4-Indoor-Runtime]]"]
tags: [mm6, f4, save, maps]
---

# MM6 F4 Map Save Fields

The [F4 map-save packet](../project/MM6_EC6_MAP_FIELDS_F4.md) describes a bounded `MAPS` section in the private `.ec6` container. It carries up to 67 named MM6 delta byte payloads and rejects duplicate or path-like names, invalid sizes, trailing data and container corruption before assigning a decoded candidate. The payloads remain opaque to this codec; the eventual whole-save transaction must validate every map against its original-matched base and atomically replace the user's save file.

The retained offline fixture covers 52 indoor and 15 outdoor deltas byte for byte. The active map still needs a changed-state capture, and the save menu, autosave and relaunch path remain disabled. Read the [F4 build graph](../project/MM6_F4_BUILD_GRAPH.md) and [save specification](../project/MM6_SAVE_SPEC.md) before extending this seam. No MM6 process or original installation was opened here.
