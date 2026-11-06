---
type: synthesis
up: "[[MM6-F4-Chest-Generation]]"
tags: [mm6, f4, chest, save, wire]
extends: "[[MM6-F4-Chest-Placement]]"
---

# MM6 F4 Chest Grid Wire

The accepted original MM6 executable writes the full 4,204-byte runtime chest record into changed indoor/outdoor map data, resolving the final 280 bytes as 140 signed little-endian grid cells.

The in-memory placement trace already used all 140 cells. A separate original map archive trace found two map-type branches that each copy the full runtime chest array without splitting the last 280 bytes; map file-writing paths use the same record width. This supersedes the F2 81-cell-plus-tail hypothesis for the accepted executable. Retained shipped deltas have only zero grid cells, so there is still no observed nonzero original-save fixture.

`mm6::decodeChestGridWire` and `encodeChestGridWire` are detached exact-width, MM6-only codecs that preserve the four-byte header and 140 item slots. Synthetic nonzero cells beyond index 80 round-trip, including through `.ec6` `MAPS` section encoding. Three focused native tests passed. The helpers do not attach to live snapshots or lift the MM6 chest generation and SaveLoad guards. Runtime grid consistency, initialized chest replay, item placement, map reset and durable save/reload still need integration and validation. Exact original instruction evidence stays in the ignored private companion.

See also: [MM6 F4 Chest Placement](MM6-F4-Chest-Placement.md), [MM6 F4 Chest Generation](MM6-F4-Chest-Generation.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
