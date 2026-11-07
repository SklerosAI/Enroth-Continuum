# MM6 first-map resume handoff — 2026-11-07

This is the bounded next gate for a new session or agent. It records the paused
state and the exact check to run; it does not claim a playable MM6 campaign.
Read [plan §18.1](plan.md) for the F4b → F5a → F4d → F4c → F5b → F6 order.

## Start from the right checkout

- Use the `mm6-mac-implementation` integration checkout reported by
  `git worktree list`. Check `git status --short --branch` before editing. The
  old `mm6-ec6-transaction` checkout is not the integration tree.
- The implementation goal is paused. At the pause, the integration tree was
  clean. The latest code-changing commit is `0d017c115`; subsequent commits
  update planning, handoff and worktree policy. Do not push the local MM6
  integration history to the public remote without the separate privacy audit
  in [Source and Privacy Boundary](../.llm-wiki/Source-and-Privacy-Boundary.md).
- Ten Git worktrees were registered at the pause. For parallel code work, use
  `python3 tools/check_worktree_budget.py` before creating another and follow
  the [agent notes](../AGENTS.md). One agent can work in the integration tree.
- Private game data, rules, saves and evidence live outside tracked Git. Find
  local evidence through the ignored `local-game-data/private-wiki/INDEX.md`.
  The ignored `mm6/RESUME_LOCAL_PATHS_2026-11-07.md` there has the exact
  machine-specific environment exports. Never copy its contents or raw game
  files into this tracked handoff.
- The ignored private wiki was copied to a local, access-restricted backup and
  all **39 files** matched a tree hash on 2026-11-07. This is a same-disk
  recovery copy, not a cloud or external-drive backup.

## Exact code state

`89fc3b913` introduced the Engine-owned `mm6::V5FirstMapLoadAttempt` and an
exact first-map chest handoff. Its fresh seven-child v5 chain passed **7/7**,
both full-retry fault children passed **1/1**, and MM7 control passed **1/1**.
`0d017c115` then moved first-map EVRT restoration into that attempt. On
2026-11-07 the gate runner passed it: seven main children **7/7**, three
fault-setup and two full-retry fault children **5/5**, and MM7 control
**1/1**, each in a fresh process.

Relevant source entry points:

| Concern | Source |
| --- | --- |
| Attempt identity, phase and exact slot/map guards | [`MM6V5FirstMapLoadAttempt.h`](../src/Engine/MM6/MM6V5FirstMapLoadAttempt.h), [`MM6V5FirstMapLoadAttempt.cpp`](../src/Engine/MM6/MM6V5FirstMapLoadAttempt.cpp) |
| Engine ownership and lifecycle | [`Engine.h`](../src/Engine/Engine.h), [`Engine.cpp`](../src/Engine/Engine.cpp) |
| Native outdoor EVRT and chest order | [`Outdoor.cpp`](../src/Engine/Graphics/Outdoor.cpp), [`MM6EventRuntimeSaveFields.cpp`](../src/Engine/MM6/MM6EventRuntimeSaveFields.cpp) |
| Disposable replay and two fault retries | [`GameTests_MM6V5AfterActionFreshChildReplay.cpp`](../test/Bin/GameTest/GameTests_MM6V5AfterActionFreshChildReplay.cpp) |
| Existing save writer and numbered-slot boundaries | [`MM6V5FirstMapSaveWriter.cpp`](../src/Engine/MM6/MM6V5FirstMapSaveWriter.cpp), [native slot packet](MM6_F4_NATIVE_FIRST_MAP_SLOT_POLICY.md) |

The attempt is currently restricted to MM6, the first outdoor map and one
disposable v5 candidate name. Its phases are `Detached → Published →
EventRestored → ChestApplied`; `Abandoned` is a refusal state. Pre-event
actor/generated-NPC/RNG, sprite and decoration replay, post-world admission,
and input decision still involve test-only scopes. Normal SaveLoad/UI must not
construct or admit the attempt yet. MM7's route remains separate.

## First action on resume: validate the compiled EVRT slice

Set `MM6_DATA_PATH`, `MM6_RULES_PATH` and `MM7_DATA_PATH` in the shell to the
existing **private local** directories, then run from the integration checkout:

```sh
python3 tools/run_mm6_firstmap_gate.py --list
python3 tools/run_mm6_firstmap_gate.py
```

The runner builds `OpenEnroth_GameTest` with the project CMake, creates
owner-only disposable profiles, runs the seven first-map v5 children in order,
then a fresh three-child setup and both full-retry fault children, and finally
the MM7 `Prs.Pr2354` control. Every child is a separate process. It requires
one passing test per child and refuses skipped tests; it removes temporary
profiles when done. It does not launch a foreground window, use a production
save, or replace the installed Atlas. The precise filters and opt-in values
are in the runner, so future agents need not reconstruct a long command list.

**Acceptance:** all seven main children, all five setup/fault children and the
MM7 control pass. If one fails, retain the failure name and last output locally;
inspect only its source path and the attempt's phase/order. Do not loosen an
identity guard or open SaveLoad to make the test green. A passing run verifies
this slice only; it does not prove normal saves, later maps or a campaign.

## Next implementation decisions after a pass

1. Move pre-event actor and generated-NPC/RNG replay from disposable scopes
   into the same Engine-owned attempt, with source-bound identity and a single
   phase transition before map events.
2. Move decoration and sprite replay into that attempt in the native source
   order. Require once-only publication and reject a changed native anchor.
3. Let the attempt own exact post-world validation and release before first
   input. Keep the fail-closed abandon-to-menu path for partial world failures;
   this is not a general rollback.
4. Connect the already tested paused Save capture and numbered `.ec6` slot
   policy to normal UI and a fresh-process normal save/reload. Only then run
   the early staged graphical F5a opening check. A one-map success remains a
   preview, not Atlas MM6 Play.

For each integrated transaction milestone, run the direct changed-path test,
the seven-child chain, the relevant fault children and MM7 control. Broader
suites belong at major readiness gates or when a specific regression appears.
Update the focused [v5 packet](MM6_F4_EC6_V5_SPRITE_RUNTIME.md),
[wiki handoff](../.llm-wiki/Agent-Handoff.md), index and separate wiki log
after significant verified work; routine edits need no duplicate status prose.
