# MM6 first-map resume handoff — 2026-11-09

This is the bounded next gate for a new session or agent. It records the
current F4b state and the exact check to run. It does not claim a playable MM6
campaign. Read [plan §18.1](plan.md) for the F4b → F5a → F4d → F4c → F5b → F6
order. Updated 2026-11-09 after production first-map load, map classification,
house services and the character-tab crash guard reached integration. The lane-by-lane record is in
[the session handoff](SESSION_HANDOFF_2026-11-07.md).

## Start from the right checkout

- Use the `mm6-mac-implementation` integration checkout reported by
  `git worktree list`. Check `git status --short --branch` before editing. The
  old `mm6-ec6-transaction` checkout is not the integration tree.
- The implementation goal resumed on 2026-11-07 with parallel lanes. Do not
  push the local MM6 integration history to the public remote without the
  separate privacy audit in
  [Source and Privacy Boundary](../.llm-wiki/Source-and-Privacy-Boundary.md).
- The repository cap is 10 registered worktrees. Run
  `python3 tools/check_worktree_budget.py` before creating another and follow
  the [agent notes](../AGENTS.md). Code-changing lanes use their own worktrees,
  never the integration checkout.
- Private game data, rules, saves and evidence live outside tracked Git. Find
  local evidence through the ignored `local-game-data/private-wiki/INDEX.md`.
  The ignored `mm6/RESUME_LOCAL_PATHS_2026-11-07.md` there has the exact
  machine-specific environment exports. Never copy its contents or raw game
  files into this tracked handoff.

## Exact code state

F4b is closed headlessly for the seeded first-map session. What runs:

- **First-map load attempt.** `mm6::V5FirstMapLoadAttempt` owns every replay
  phase: `Detached`, `Published`, `EventRestored`, `WorldPublished`,
  `ChestApplied`, `GeneratedNpcsKept`, `RngRestored` and `PostWorldAgreed`.
  Actors, generated NPCs, decorations and sprites are swapped in as one
  once-only phase before map events. `mm6::releaseV5FirstMapLoadAttempt()`
  runs the post-world check from `Game::gameLoop` before first input and
  abandons to the main menu on refusal.
- **Save menu.** The normal MM6 Save menu writes the selected or first free
  `saveNNN.ec6` into the native MM6 profile through the numbered-slot policy
  and the atomic writer. A refused or failed write keeps the menu open with a
  status message and leaves the old slot byte-identical.
- **Load menu listing.** The Load menu lists only loadable numbered slots of
  the native MM6 profile and hides malformed, backup-only, other-map and
  foreign files. In-game load is refused.
- **Production first-map load and resave.** The MM6 branch of `loadGame()`
  admits numbered first-map slots through `mm6::loadFirstMapManualSlot`.
  Fresh processes load through the normal menu, move and save again; the old
  primary becomes `.bak`. The full gate passed 25 child checks on 2026-11-09,
  including three MM7 controls.

What does not run yet:

- **Production New Game save, quicksave, quickload, autosave, cross-map
  travel and saves of any map other than the first** stay refused.
- **F5a** has not started.

Relevant source entry points:

| Concern | Source |
| --- | --- |
| Attempt phases, release and abandon | [`MM6V5FirstMapLoadAttempt.h`](../src/Engine/MM6/MM6V5FirstMapLoadAttempt.h), [`MM6V5FirstMapLoadAttempt.cpp`](../src/Engine/MM6/MM6V5FirstMapLoadAttempt.cpp), `Game::gameLoop` in [`Game.cpp`](../src/Application/Game.cpp) |
| Engine ownership and world preparation | [`Engine.h`](../src/Engine/Engine.h), [`Engine.cpp`](../src/Engine/Engine.cpp), [`Outdoor.cpp`](../src/Engine/Graphics/Outdoor.cpp) |
| Save and Load menus | [`MM6FirstMapManualSave.h`](../src/Engine/MM6/MM6FirstMapManualSave.h), [`MM6SaveLoadMenu.h`](../src/GUI/UI/MM6SaveLoadMenu.h), MM6 branches of [`SaveLoad.cpp`](../src/Engine/SaveLoad.cpp) and [`UISaveLoad.cpp`](../src/GUI/UI/UISaveLoad.cpp) |
| Slot policy and writer | [`MM6V5FirstMapNativeSlotPolicy.h`](../src/Engine/MM6/MM6V5FirstMapNativeSlotPolicy.h), [`MM6V5FirstMapSaveWriter.cpp`](../src/Engine/MM6/MM6V5FirstMapSaveWriter.cpp), [native slot packet](MM6_F4_NATIVE_FIRST_MAP_SLOT_POLICY.md) |
| Gate children | [`GameTests_MM6V5AfterActionFreshChildReplay.cpp`](../test/Bin/GameTest/GameTests_MM6V5AfterActionFreshChildReplay.cpp), [`GameTests_MM6V4FreshChildReplay.cpp`](../test/Bin/GameTest/GameTests_MM6V4FreshChildReplay.cpp), [`GameTests_MM6V5NormalLoadMenu.cpp`](../test/Bin/GameTest/GameTests_MM6V5NormalLoadMenu.cpp) |

## The gate

Set `MM6_DATA_PATH`, `MM6_RULES_PATH` and `MM7_DATA_PATH` in the shell to the
existing **private local** directories, then run from the integration checkout:

```sh
python3 tools/run_mm6_firstmap_gate.py --list
python3 tools/run_mm6_firstmap_gate.py
```

The runner builds `OpenEnroth_GameTest` with the project CMake (it refuses a
Homebrew CMake), then runs four fresh profiles in order:

1. The seven-child first-map v5 chain.
2. Seed, v4 and v5 setup children, then three full-retry fault children:
   partial assignment, pre-event fault and post-world mismatch.
3. The same setup, then the paperdoll child.
4. Seed and v4, then the normal Save menu child and the normal Load menu child.

It ends with three MM7 controls. Every child is a separate process
and must report one passing test. Skipped tests count as failures. Temporary
owner-only profiles are removed afterward. Under load, raise the per-child
timeout with `ENROTH_GATE_CHILD_TIMEOUT`. The Save and Load menu children
already get at least 1500 s. The runner never launches a foreground window,
writes a production save or replaces the installed Atlas.

The full gate passed after the A and B merges, with the MM7 save candidate
integrated (see the [wiki log](../.llm-wiki/log_Enroth-Continuum.md)). The G
and P lanes reported a passing gate after each of their commits.

**Acceptance:** every child and the MM7 control pass. If one fails, keep the
failure name and last output locally, then inspect only its source path and
the attempt's phase and order. Do not loosen an identity guard or open
SaveLoad to make the test green. A pass verifies the first-map route only. It
does not prove later maps, travel or a campaign.

## Next steps

1. Run F5a, the staged graphical opening check, before broadening the
   campaign. It needs no code ownership, only a capture and a report.
2. Then the F4d backlog in the session handoff: transition wiring per the
   [wiring plan](MM6_TRANSITION_WIRING_PLAN.md), party death wiring, shops and
   guilds (Lane M was started), house NPC hiring, transport, calendar start
   year and reputation, and the remaining spells and combat once Codex hands
   back the shared combat files.

For each integrated milestone, run the changed-path test, the full gate and
the MM7 control. Broader suites belong at major readiness gates or when a
specific regression appears. The current per-row evidence is in the
[implementation checklist](MM6_IMPLEMENTATION_CHECKLIST.md). Update the focused
[v5 packet](MM6_F4_EC6_V5_SPRITE_RUNTIME.md), the
[wiki handoff](../.llm-wiki/Agent-Handoff.md), the index and a separate wiki
log after significant verified work. Routine edits need no duplicate status
prose.
