# MM6 first-map resume handoff — 2026-11-09

This is the bounded next gate for a new session or agent. It records the
current F4b state and the exact check to run. It does not claim a playable MM6
campaign. Read [plan §18.1](plan.md) for the F4b → F5a → F4d → F4c → F5b → F6
order. Updated 2026-11-09 after production first-map load, map classification,
house services and the character-tab crash guard reached integration. The lane-by-lane record is in
[the session handoff](SESSION_HANDOFF_2026-11-07.md).

The MM6 character record now uses its own five tab/exit buttons and title
specific art names. Three headless checks cover archive-backed icon names, key and button
navigation, and the adjacent-frame key race. The full first-map gate passed
25/25 after integration. Exact visual placement remains for F5a.

The later MM6 base paperdoll slice resolves body, arm and hand art for all
twelve faces. Four focused character checks and one MM7 inventory control
passed; a fresh hidden character capture passed 1/1. Other equipped-item layers,
the remaining inherited panel resource, exact tab placement and visible
input/save verification remain F5a work. This does not change the 25-child
first-map result or open a normal New Game route.

The next armor slice maps every active MM6 armor template to retained body and
arm layers. The integrated MM6 character suite passed 6/6, including a
clickable equipped-armor area; an MM7 inventory control and a monitored hidden
armor capture passed 1/1 each. Four public variant rows absent from the
original archive are not requested. Other equipment layers, the inherited
panel lookup, exact alignment and visible input/save acceptance remain open.

The next MM6 paperdoll slice uses native item-table positions for main-hand
weapons, bows and shields and public extra-hand offsets for dual wielding.
Three inherited artifact assertions were removed. The integrated character
suite passed 9/9; an MM7 inventory control and monitored hidden weapon
capture passed 1/1 each. Two ordinary daggers use an inferred translated
offhand position pending original-frame comparison. The wearable route is
recorded below; cloaks, panel art and visible input/save acceptance remain open.

Boots, helms and belts now use MM6 item positions and retained art, with
public paperdoll substitutions for helms and belts. The integrated MM6
character suite passed 11/11, an MM7 inventory control passed 1/1, and a
hidden wearable capture passed 1/1 without a sampled visible window or focus
change. Cloaks, panel art, exact alignment and visible input/save acceptance
remain open.

Equipped MM6 cloaks now use retained cape or generic art at native item-table
positions behind the base figure. The integrated character suite passed 13/13,
including specific and generic cloak hit areas; the MM7 inventory control and
monitored hidden cloak capture passed 1/1 each. The monitor sampled 124 times
without a visible window or focus change. The inherited panel lookup, exact
alignment and visible input/save acceptance remain open.

The inherited `ib-mb-A` right-panel border is MM7-specific and is no longer
requested for MM6. All MM6 consumers skip the absent border. The integrated
character/house checks passed 14/14, the MM7 inventory control passed 1/1,
and a hidden character capture passed 1/1 over 87 samples without a visible
window or focus change. The log no longer requests that border. Other UI icon
requests, original alignment and visible input/save acceptance remain open.

The MM6 selected-portrait HUD now cycles through six retained sprite frames
with the UI animation clock and per-frame palettes. In integration, two
focused MM6 checks, four MM7 controls and a monitored hidden two-frame render
passed; the monitor saw no visible window or focus change in 98 samples.
Party creation still uses the first frame. Original visible timing and
alignment remain unverified.

The first-map HUD's four baked book controls now open and close through a
fresh-process headless check. The common MM6 book frame and exit tab, plus
the Quest page background and navigation tabs, use original source-traced
positions and retained MM6 art. The integrated MM6 book and Calendar checks
passed 1/1 each; an MM7 quest-book control passed 1/1. Notes and Map still
request inherited art, and visible page layout remains open for F5a.

## Start from the right checkout

- Use the `master` integration checkout reported by `git worktree list`.
  Check `git status --short --branch` before editing. Older MM6 branch refs
  are historical checkpoints, not the active integration tree.
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
- **F5a** has hidden world and character-window diagnostics with title-specific
  paperdoll equipment and the inherited right-panel border request resolved.
  Other UI icons, original alignment and visible opening remain open.

The opt-in `tools/run_mm6_firstmap_gate.py --stage-f5a-profile` mode builds
the GameTest and MM6 app, runs five fresh-process seed, save and Load checks,
then retains a mode-0700 system-temporary MM6 profile with one loadable
`save000.ec6` slot and local rules. Supply private `MM6_DATA_PATH` and
`MM6_RULES_PATH` environment variables. It prints the exact MM6 app command
using `--data-path` and `--user-path`, plus the temporary profile cleanup
command. This is a prepared **Load** check; production New Game is still
guarded, and no visible F5a result has been recorded.

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

The pause checkpoint is integration `20e6f3491`, clean and pushed. The clean
`codex/mm6-notes-book` worker branch contains the same Quest runtime change
and is ready for the next code slice; find it with `git worktree list`.

1. Finish the MM6 Notes page resource route in
   `src/GUI/UI/Books/AutonotesBook.cpp`. The retained `note_bg` is 360×300;
   the identity-matched original draws it at (47,22). Its previous/next tabs
   use the same MM6 art and positions as Quest. The original has five native
   category tab pairs at x415 active/x418 inactive, with y values
   118/153/188/223/258. The retained notes table has no `teacher` rows,
   while the inherited screen creates a sixth Instructors tab. Confirm the
   five icon-to-category meanings from the original route before replacing
   the inherited tabs; keep original assets and detailed traces local. Then
   run the focused fresh-process book check (which currently passes but logs
   missing Notes and Map art) and MM7 `Issues.Issue2903` as the title control:

   ```sh
   build/test/Bin/GameTest/OpenEnroth_GameTest --game mm6 --headless --data-path "$MM6_DATA_PATH" --rules-path "$MM6_RULES_PATH" --test-path test/data --gtest_also_run_disabled_tests --gtest_filter=MM6CalendarHeadless.DISABLED_FirstMapHudBooksOpenAndClose --gtest_brief=1 --log-level=error
   ```

   Accept Notes only when its native resources resolve and the real first-map
   buttons still open and close; a passing test with missing-icon logs is not
   visual acceptance. Map remains a separate art/layout slice.
2. Continue F5a with the isolated profile staged by
   `python3 -B tools/run_mm6_firstmap_gate.py --stage-f5a-profile`. Select
   Load, compare the visible opening and paperdoll alignment with the
   original, exercise input and Save/quit/relaunch/Load, and record defects.
   The on-screen check has not run and awaits the owner's answer because it
   may take focus. Production New Game remains guarded. Keep hidden captures
   separate from visible acceptance and delete the temporary profile after
   the check.
3. Then the F4d backlog in the session handoff: transition wiring per the
   [wiring plan](MM6_TRANSITION_WIRING_PLAN.md), party death wiring, shops and
   guilds, house NPC hiring, transport, reputation, and the remaining spells
   and combat.

For each local edit, run the changed-path test; run the full first-map gate at
transaction milestones and the MM7 control after shared-core changes. Broader
suites belong at major readiness gates or when a specific regression appears.
The current per-row evidence is in the
[implementation checklist](MM6_IMPLEMENTATION_CHECKLIST.md). Update the focused
[v5 packet](MM6_F4_EC6_V5_SPRITE_RUNTIME.md), the
[wiki handoff](../.llm-wiki/Agent-Handoff.md), the index and a separate wiki
log after significant verified work. Routine edits need no duplicate status
prose.
