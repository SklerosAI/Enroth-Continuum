# Session handoff — 2026-11-07 (parallel MM6 lanes and MM7 audit)

Purpose: let a fresh session or a different model pick up the orchestration
without the chat transcript. Update this file as lanes report. Authority order
stays as in `AGENTS.md`: source and tests over packets, packets over this file.

## Owner direction from this session

- Priority: MM6 implementation lanes, then the MM7 inherited-engine audit and
  its fixes, then MM7 private mods ([MM7_MODS_PLAN.md](MM7_MODS_PLAN.md)).
- The two-lane agent cap in `AGENTS.md` is lifted for this work. Hard limits
  that remain: at most 10 registered worktrees, at least 10 GB free disk.
- No Docker or devcontainer. Visible game windows only when really useful, on
  the built-in main display. Headless and hidden offscreen runs are fine.
- Every game run gets a capture through `~/enroth-tools/bin/enroth-capture`,
  saved to the ignored `local-game-data/progress-captures/`, newest 25 kept.
- End state: one worktree, with MM6 and MM7 as separate engine code sections
  under full project control rather than an OpenEnroth engine with title
  checks (see [plan §18](plan.md)). Until the post-MM6 split refactor, new MM6
  logic goes in MM6-owned files with thin dispatch hooks in shared code.
- Save results as Markdown as they arrive so another model can continue.
- The owner is not attached to old MM7 saves. Delete rather than Trash when
  retiring OpenEnroth leftovers.

## Local tooling added this session (outside Git)

- `~/enroth-tools/bin/enroth-build <target>...`: machine-wide build throttle
  (3 concurrent builds, niced, `-j4`). `ENROTH_BUILD_DIR` selects a build dir.
- `~/enroth-tools/bin/enroth-capture <label> [--mm7|--from png|--screen]`:
  hidden offscreen first-map render with the current worktree's GameTest, or a
  copied PNG, or a main-display screenshot. `--screen` needs macOS Screen
  Recording permission for VS Code, which is not granted yet.
- `~/enroth-tools/bin/ccache` 4.14.2, with shared settings in
  `~/enroth-tools/env.sh` (`CCACHE_DIR=~/enroth-tools/ccache`, `BASEDIR=$HOME`,
  `NOHASHDIR`, 8G). A build dir only uses it after `cmake -UCCACHE_FOUND build`.
- `env.sh` `OPENENROTH_MM7_PATH` now points at `Game Data/MM7`.

## MM6 first-map gate

`tools/run_mm6_firstmap_gate.py` passed on `9883de1b7`: seven-child chain 7/7,
five setup and fault children 5/5, MM7 `Prs.Pr2354` 1/1. Its temp profile
prefix was fixed to `mm6-restart-` in `27ce8f61f`.

## Running MM6 code lanes

All branch from `9883de1b7`. Shared brief: the lane rules file used for the
spawn prompts required `enroth-build`, `enroth-capture`, file ownership and the
MM6-owned-file placement rule. No lane had committed when this file was
written. Readiness audits behind these lanes:
[MM6_F4D_READINESS_AUDITS_2026-11-07.md](MM6_F4D_READINESS_AUDITS_2026-11-07.md).

| Lane | Branch / worktree under `~/.codex/worktrees/` | Scope | Owns |
| --- | --- | --- | --- |
| A | `mm6-f4b-firstmap-attempt` | Move pre-event actor/GNPC/RNG, sprite/decoration and post-world phases into `mm6::V5FirstMapLoadAttempt` | attempt files, `Engine.*`, `Outdoor.cpp`, v5 tests |
| B | `mm6-f4b-normal-save-ui` | Normal MM6 Save and Load UI with numbered `.ec6` slots and a fresh-process reload chain | `SaveLoad*`, `UISaveLoad.cpp`, live capture, save writer |
| H | `mm6-f4d-house-services` | House entry hardening, service dispatch, temple heal, house NPC dialogue | `UIHouses.cpp`, `UIDialogue.cpp`, `NPCTopics.cpp`, `NPC.cpp`, `Houses/*` |
| E | `mm6-f4d-evt-vars` | Remaining event variables and opcodes, calendar start year | `Evt/MM6*`, interpreter variable cases, `Time.h` |
| T | `mm6-f4d-transition-owners` | Detached visited-map cache, indoor visited owner, combined codec, border confirmation, title-aware `isMapIndoor` helper, wiring plan | new MM6 files, `MapEnumFunctions.h` |
| I | `mm6-f4d-inventory` | INVR equip and move transitions, MM6 equip rules, cursor/HELD sync, record add/remove | MM6 inventory projection files, inventory and equip functions |
| S | `mm6-f4d-spells-combat` | Reject-before-mutate ordering, Electricity/Cold damage, first-map monster spells 2/4/18/24, break-any special attack, player cast allowlist from spell 67 | spell guard, monster damage/spell files, spell and damage regions of shared files |

### Lane state at the stop (17:20 local)

All seven lane agents stopped at an account usage limit, mid-task. None
committed. Their edits are uncommitted in their worktrees on top of
`9883de1b7` and must be preserved. Untracked new files are part of the work,
so `git status` (not only `git diff`) shows the full change set. None of this
was compiled together, and no lane's tests are known to pass at this snapshot.

| Lane | Tracked diff | New untracked files | Last known point |
| --- | --- | --- | --- |
| A | 8 files, +132/-155 | `MM6PreEventActorReplay.{h,cpp}` | Step 1 (pre-event actor replay into the attempt) had a gate run and a capture at 13:47. Steps 2-3 not started or not confirmed. |
| B | 15 files, +322/-21 | `MM6FirstMapManualSave.*`, `src/GUI/UI/MM6SaveLoadMenu.*`, `GameTests_MM6V5NormalLoadMenu.cpp` | Fixing a compile error (missing include for `V5FullSessionPreflight`). Save menu, load list and load route in progress. |
| H | 8 files, +124/-71 | `Houses/MM6House.*`, `Houses/MM6Temple.*`, `MM6Temple_ut.cpp`, `GameTests_MM6Houses.cpp` | Waiting on a build. Entry hardening and temple work in progress. NPC dialogue step not started. |
| E | 6 files, +148/-99 | `Evt/MM6{CircusPrize,DayCounter,EventVariableDispatch,Hireling,Resistance}Event.*` with unit tests, `GameTests_MM6EventVariables.cpp` | Variable dispatch moved to MM6 files. Build and test state unknown. |
| T | 14 files, +1471 (all new or helper) | none beyond the tracked list | Visited-map cache, visited indoor owner, combined section codec, border confirmation and `isMapIndoor` helper with unit tests written. Wiring plan not yet reported. |
| I | 12 files, +305/-25 | `MM6PaperdollEquip.{h,cpp}` | Projection, HELD sync and paperdoll equip in progress. Also edits the v5 replay test that Lane A edits. |
| S | 16 files, +264/-106 | `MM6CombatRules.h`, `MM6MonsterSpellEffect.h`, `GameTests_MM6Combat.cpp` | Reject-before-mutate ordering and damage channels in progress. Touches `Actor.cpp`, `Character.cpp`, `Game.cpp`, `EvtInterpreter.cpp`, `SpriteObject.cpp`. |

A stopped lane agent can be resumed with its own context through a message to
it once the limit resets. A new session should instead read the worktree diff,
build it through `enroth-build`, and continue from the table above.

Integration order, set by the MM6 lead and sent to Codex in
`agent_comms/messages/2026-11-07T22-23-28Z--from-claude-mm6-lead--to-codex--mm6-assignment.md`:
T, E, A, B, I, H, S, then Codex C1 and C2. One lane at a time, focused tests,
the first-map gate and MM7 controls after shared-code merges, then retire the
worktree. Hotspots are in `agent_comms/reviews/codex-mm6-lane-overlap-2026-11-07.md`.

The combined F4b result is shown by `tools/run_mm6_firstmap_gate.py` extended
with Lane B's normal UI section: New Game, bounded play, Save through the menu
into a numbered `.ec6` slot, a fresh-process Load through the menu and first
input, each child 1/1, plus the existing chain, fault children and the MM7
controls `Prs.Pr2354`, `Prs.Pr2956` and `Issues.Issue840`.

## Codex coordination

Codex works through `agent_comms/` (read its `README.md`). Codex was assigned
two MM6 tasks that avoid every lane-owned file: C1, an MM6 indoor transition
dialog in new `src/GUI/UI/MM6TransitionDialog.*` with a thin dispatch in
`UITransition.cpp`, and C2, a detached MM6 party death model in
`src/Engine/MM6/MM6PartyDeath.*`. Codex also owns isolated MM7 work: mood
portraits (`b6a1515c3`), an atomic MM7 save-slot helper (`875be4faf`,
`a3da4586d`), the actor-10 turn-queue fix (`73feee2b2`), the MM7 artifact fix
(`bbd7cc2f8`..`fe5ce5499`) and the house pause fix with `Prs.Pr2956` and an
audio pause tape (`c11c2c1f9`). Shared SaveLoad/UI and combat files stay with
the MM6 lanes until handed off.

## MM7 audit

Results: [MM7_AUDIT_RESULTS_2026-11-07.md](MM7_AUDIT_RESULTS_2026-11-07.md).
A0 to A2 are done statically. Next: write game tests for the top defects and
fix them in a dedicated MM7 audit branch once a worktree is free.

## MM7 Atlas state

- `/Applications/Enroth Continuum.app` carries one Release `MM7.app` helper
  from the integration tree's `MM7` target, with profile `Profiles/MM7-Atlas`
  (seeded with the previous settings and all earlier saves). The old helper,
  `~/.openenroth`, `Application Support/OpenEnroth`, `Profiles/MM7` and the
  nightly DMG were deleted.
- Known defect in that install: it links XQuartz `libGL`. In progress:
  `build-release` is reconfigured with the SDK OpenGL framework for GL and GLU
  and with ccache. Rebuild with `ENROTH_BUILD_DIR=build-release enroth-build MM7`,
  assemble with `launcher/build.sh --mm7-app build-release/src/Bin/OpenEnroth/MM7.app
  --output build-release/Atlas/"Enroth Continuum.app"`, check `otool -L` shows
  no `/opt/X11`, then replace the installed app.
- The MM7 house pause fix landed as `c11c2c1f9` (with `Prs.Pr2956`). The
  pre-fix binary passed `Issues.Issue840`, which does not tape the pause. The
  installed helper predates the fix, so the next Release rebuild carries both
  the pause fix and the OpenGL framework link.
- Remaining MM7 audit fixes: see the ranked lists in
  `MM7_AUDIT_RESULTS_2026-11-07.md` (Break Weapon inversion, Divine
  Intervention recovery, lich immunity, Divine Restoration target, archer
  projectile token first).

## Worktrees (11 registered, cap is 10)

Main checkout, `mm7-frozen` (`mm7-isolated-runtime`), `mm8-isolated`
(`mm8-isolated-runtime`, fully merged, removal was refused by the safety
classifier and needs explicit owner approval), the seven lane worktrees above,
and Codex's `mm7-save-atomic` (branch `codex/mm7-artifact-cap`, clean,
integrated by subject). Codex was asked to reuse or retire it. Seven older
worktrees were retired this session after confirming their commits were
integrated. Their ignored private files are archived under
`local-game-data/worktree-archive-2026-11-07/`.

## Next steps for whoever resumes

1. Get back under the worktree cap (Codex's checkout, see above).
2. Lane T first: build its worktree through `enroth-build` (run
   `cmake -UCCACHE_FOUND build` once to enable ccache), run the MM6 unit
   tests it added, `check_style`, commit in the lane with a full message,
   rebase onto `mm6-mac-implementation`, merge, rerun the first-map gate.
3. Then E, A, B, I, H, S in that order, same procedure.
4. Rebuild the Release `MM7.app` from the integration tree after the lanes
   that touch shared code have merged, check `otool -L`, reinstall the Atlas.
5. Keep this file and the MM7 audit results current as work lands.
