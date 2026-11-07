---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Held-Item]]", "[[MM6-F4-New-Game-Autosave]]"]
tags: [mm6, save, transaction, actions]
---

# MM6 F4 Action Observation Stability

The [source audit](../project/MM6_F4_ACTION_OBSERVATION_STABILITY.md) explains why the current idle-action observation does not yet protect a real map-entry save transaction.

The MM6-only test wrapper rejects queued spells, targeting, turn/rest state, both GUI message frames, movement actions and delayed enchant messages before reading the private map source. Its present archive readers and atomic writer do not explicitly pump input. The scoped runner now calls its observer on staged Party/map/CHST candidates before writing or publishing anything, and the wrapper rechecks pending actions immediately afterward. A newly queued action rejects the save and remains queued. A full map load may still render or process events, and those action/UI/world effects have no rollback owner. The GameTest control mutex prevents simultaneous game/control execution but permits a handoff at `swapBuffers()`.

A second read before disk write now catches actions introduced by the bounded precommit callback. No callback runs after write in this test seam. This is still not a production lock: the real active world must be built on detached owners and a coordinator-owned game-thread safe point must enforce no reentry until every owner commits, or a complete staged/no-throw rollback must cover action, UI and world effects. The linked save/world suite passed 14/14; a monitored private-data GameTest injected a next-frame GUI action during precommit and passed 1/1 with no visible window or focus change, confirming no disk/cache/CHST publication. A late full-world failure and save–quit–reload still need proof. The normal MM6 SaveLoad guard remains closed.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Held Item](MM6-F4-Held-Item.md), [New Game Autosave](MM6-F4-New-Game-Autosave.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
