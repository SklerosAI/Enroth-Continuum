---
type: synthesis
up: "[[MM6-F4-Offline-Seams]]"
related: ["[[Source-MM6-Opcode-Side-Effects-F2]]", "[[MM6-F4-Original-Noop-Events]]"]
tags: [mm6, f4, events, indoor, facets]
---

# MM6 F4 Indoor Facet Events

Original MM6 indoor texture and face-bit events address one face by global index, while the inherited MM7 helpers address a cog group.

A read-only trace of the F3-matched staged executable shows opcode 11 checks for an indoor level and a bounded signed face index before selecting a texture name for that face. Opcode 23 performs the same target check, then sets or clears the supplied 32-bit mask on that face. Invalid targets and nonindoor execution advance without mutation. The retained active event corpus contains 215 texture and 32 face-bit records. The detailed source addresses and operands stay in the ignored local companion.

The MM6 interpreter now checks `pIndoor->faces[index]` for these two commands, while MM7 keeps its existing cog-group route. The synthetic headless check covers one selected face despite identical cog numbers on three faces, set/clear, texture, invalid index and outdoor no-op. Integrated arm64 targets linked and the focused monitored headless check passed **1/1** over 306 samples without a visible window or focus change.

This is a target-selection correction. The [persistence gate](../project/MM6_F4_INDOOR_FACE_PERSISTENCE_GATE.md) found no explicit face array in the MM6 DLV and no `location.faces` capture in the current changed-state writer. Among seventeen affected active maps, six have no map-reload event; the other eleven still need branch and state checks. The opaque DLV blocks have unestablished meanings, so neither original persistence nor an `.ec6` replay rule is proved. Original texture animation lookup, changed-face save/reload, visual result and campaign path remain open. Normal MM6 SaveLoad and chest generation are still guarded.

See also: [MM6 F4 Offline Seams](MM6-F4-Offline-Seams.md), [MM6 Opcode Side Effects F2 Source](Source-MM6-Opcode-Side-Effects-F2.md), [MM6 F4 Original No-Op Events](MM6-F4-Original-Noop-Events.md).
