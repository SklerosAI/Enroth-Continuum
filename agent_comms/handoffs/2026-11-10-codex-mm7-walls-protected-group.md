# MM7 Walls of Mist protected-group investigation

- Owner: Codex, branch `codex/mm7-walls-protected-group` from integration `e23b20de9`.
- Outcome: investigation complete; no runtime or permanent test change is justified by the available evidence. The branch ref is retained for the evidence handoff.

## Classification

The Walls exit's group-1 kill check is a **negative branch**: when it succeeds, the event skips the quest-pass write. The no-kill path writes the pass bit and can leave the map. The integrated input-driven `MM7LightWalls.LightRoute` proves that success path.

The owner's local MM7 map has no group-1 actor at fresh entry. A private, in-memory comparison found no group-1 actor in the original indoor delta and no group-1 actor spawn point in the base map; the loaded actors retain the other group used by that map. The same local event program contains no later group-1 monster summon or actor-group remap. This makes the protected-kill negative branch unreachable in the tested local data, but it does **not** block ordinary Light-path completion. It may be a dormant script condition or an original data inconsistency; these checks do not establish a runtime decoding defect or a safe replacement group.

Public-code anchors: `EvtInstruction.cpp` parses the kill policy, group and required death count; `EvtInterpreter.cpp` dispatches it; `Actor.cpp` compares the actor's group directly and requires a death; `EntitySnapshots.cpp` preserves the saved actor group, `CompositeSnapshots.cpp` reconstructs the indoor actors and base spawn points, and `Indoor.cpp` spawns the base points on respawn. The map-specific `Engine.cpp` handling changes treasure and the key-door reload state, not actor groups.

## Checks and limits

- A temporary, uncommitted headless GameTest on a fresh Light-active Walls entry passed 1/1. It compared original base/delta group presence with loaded actor presence and confirmed the group-1 kill predicate is false. The diagnostic test and its CMake registration were removed after the check.
- The integrated input-driven `MM7LightWalls.LightRoute` passed 1/1 headlessly in this worktree.
- Static decoding of the relevant local MM7 event control flow confirmed the failure-branch direction. No raw event records, map data, saves, paths, or detailed asset inventory were copied into Git or this handoff.

The next meaningful check, if a different owned MM7 edition or an original-game observation raises a contradiction, is to compare its group behavior privately and verify which monsters the original negative branch protects. Do not change runtime group matching or relabel map actors from this evidence alone.
