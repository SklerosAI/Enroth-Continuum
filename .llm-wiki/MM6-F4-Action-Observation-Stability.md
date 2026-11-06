---
type: synthesis
up: "[[MM6-F4-Save-Transaction-Checklist]]"
related: ["[[MM6-F4-Held-Item]]", "[[MM6-F4-New-Game-Autosave]]"]
tags: [mm6, save, transaction, actions]
---

# MM6 F4 Action Observation Stability

The [source audit](../project/MM6_F4_ACTION_OBSERVATION_STABILITY.md) explains why the current idle-action observation does not yet protect a real map-entry save transaction.

The MM6-only test wrapper rejects queued spells, targeting, turn/rest state, both GUI message frames, movement actions and delayed enchant messages before reading the private map source. Its present archive readers and atomic writer do not explicitly pump input. After the file, default map cache and CHST are published, however, `runScopedFirstMapEntryForTest()` calls an arbitrary entry probe. Only disk, cache and CHST are restored on failure. A full map load may render or process events, and those action/UI/world effects have no rollback owner. The GameTest control mutex prevents simultaneous game/control execution but permits a handoff at `swapBuffers()`.

A second read before disk write is useful diagnostic evidence but cannot hold the action owners stable through an arbitrary callback, and a read after failure cannot restore them. The smallest safe production prerequisite is a coordinator-owned game-thread safe point with enforced no-reentry until all owners commit, or complete staged/no-throw rollback of every action and UI effect touched during entry. Then inject a queued action during preflight and a late entry failure and prove exact process and disk restoration. The normal MM6 SaveLoad guard remains closed.

See also: [Save Transaction Checklist](MM6-F4-Save-Transaction-Checklist.md), [Held Item](MM6-F4-Held-Item.md), [New Game Autosave](MM6-F4-New-Game-Autosave.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
