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

### Lane reports after the resume

- **E, event variables: merged as `91443d377`** (lane commit `dda7dc192`).
  Variables 46-55 (resistances), 214 (NPC in party), 216-221 (day counters),
  223 (hired profession compare and dismiss), 224 (circus prizes compare), 225
  (skill points) and all 36 condition writes run from MM6 dispatch files, with
  rules read from the original variable jump tables. Lane tests: 22/22 focused
  unit, full unit suite with 0 failures, 6/6 MM6 headless, MM7 3/3, and the
  new test fails with the branches switched off. Open: variable 215
  (reputation) needs a party field and save section, opcodes 41 and 34 need
  the MM6 item generator and object descriptors, opcode 21 stays with spells,
  MM6 starts in year 1165 not 1168. The MM7 portrait sparkle helpers crash on
  MM6 data, so MM6 writes play only the sound and reaction speech.
- **S, spells and combat: done, not yet merged** (lane commits `3fffc842c`,
  `bcda1bc2c`, `dc93a3b42`). Reject-before-mutate ordering for party, monster
  and monster-on-monster hits. Original MM6 Electricity and Cold resistance
  halving for characters. Monster spells 2, 4, 18, 24 as single projectiles
  with original monster damage. `BrkItem` effect in an MM6 helper. Cure
  Weakness (67) castable from the spellbook and quick cast, other spells refuse
  with a status message. Three `DISABLED_` MM6 game tests through the real New
  Game flow, each run alone, pass with the fix and fail without it. Unit suite
  0 failures, MM7 `Prs.Pr2354` 1/1, full MM7 suite not run yet. Decision for
  the owner: vanilla MM6 made every monster spell Fire damage (GrayFace calls
  it a bug), the lane uses each spell's own element, one line to revert. Open:
  MM6 party death, inherited monster AI, monster missiles, other elements and
  spells, the second-attack bonus, the MM6 portrait animation mapping.
- **T, transition owners: done, not yet merged** (lane commits
  `91d50de34`, `d600846b2`, `d9e392f0b`, `6f6aa1038`). Title-aware map type
  helpers, visited-indoor owner, visited-map cache with a `VIST` v2 session
  codec, and the border confirmation transaction, all detached. Unit suite
  1028 passed, revert check confirmed. Wiring plan and blockers:
  [MM6_TRANSITION_WIRING_PLAN.md](MM6_TRANSITION_WIRING_PLAN.md).
- **I, inventory: done, not yet merged** (lane commits `ce7a49bba`,
  `2cf8cf000`, `a7fbbe933`, `c858984b9`). Capture writes each item's raw
  28-byte record at its current record and slot, so moves between backpack and
  paperdoll, slot and record changes, arrivals and removals save. A vacated
  record clears bytes 0-24 and keeps 25-27. `Party::setHoldingItem` and
  `takeHoldingItem` keep the raw HELD owner in step through one-line MM6
  dispatches. MM6 paperdoll rules in new `MM6PaperdollEquip.{h,cpp}` from the
  original executable: no race, alignment, spear or wetsuit checks, club and
  misc need no skill, ring placement and ring-6 swap, three record-leaking
  original cases refuse. Lane B needs no capture change. New child
  `MM6V5PaperdollHeadless.DISABLED_BackpackItemStaysEquippedThroughInventoryCapture`
  runs after SEED, V4 and V5_WRITE with `ENROTH_MM6_V5_FIRST_INPUT_ENABLE`,
  passes 1/1 and fails with the fix reverted. Unit suite passes after one
  test was updated for allowed removal. MM7 `Prs.Pr2354` 1/1. Full gate and
  full MM7 suite not run. Open: tail bytes of reused records, consume and sell
  removal untraced, backpack swap with an item in hand, auto-placement order,
  items with no raw record still refuse a save.
- **H, house services: done, not yet merged** (lane commits `66598df4d`,
  `353023478`, `d5402de3c`, `4e26b0a28`). House entry refuses cleanly on any
  missing table, manifest, icon or movie before state changes. A room with an
  NPC opens even without movie or portrait. The MM6 house screen lists the
  proprietor's services from `houseServiceOptions` and resident NPC topics.
  Temple heal follows the original: per-temple restrictions by house ID, price
  Val x condition multiplier x (condition start day mod 7, plus 1). Other
  services refuse without changing the party. Party creation no longer flags
  MM6 NPC 3 as hired (an MM7-only assumption that hid an MM6 town hall NPC).
  Tests: 4/4 MM6 house game tests (two fail with the fix reverted), 26/26 MM6
  batch, unit suite clean, 29/29 MM7 house tests. Open: training (discount
  helper untraced), donate, heal audio, opening hours, exact service menus,
  hiring topics, save/reload of house state, visual check of the screen.
- **G, MM6 item generator and give-item: started** in the old Lane E
  worktree on branch `mm6-f4d-item-generator`.

- **A, first-map attempt phases: done, not yet merged** (lane commits
  `03c55ddd9`, `15cf3af77`, `71bab617f`). Attempt phases are now Detached,
  Published, EventRestored, WorldPublished, ChestApplied, GeneratedNpcsKept,
  RngRestored, PostWorldAgreed. Actors, generated NPC rows, decorations and
  sprites are staged in native order against rebuilt anchors and swapped in
  as one once-only phase. Generated NPCs and RNG restore move into the
  attempt. `mm6::releaseV5FirstMapLoadAttempt()` runs the post-world check
  from `Game::gameLoop` after `DoPrepareWorld` and abandons to the main menu on
  refusal. `MM6V5FirstMapLoadAbandonForTest.*` is deleted. New fault child
  `MM6V5FirstMapAbandonHeadless.DISABLED_PostWorldMismatchThenFullReloadAndInput`
  is in the gate runner. Gate per step: 7/7 chain, 5/5 then 6/6 setup and
  fault, MM7 1/1, with a 1200 s scratch timeout under load. Revert checks
  fail as expected. Lane B's three entry points are not done: numbered-slot
  attempt construction (proposed `fromNativeSlot(bytes, name, title)`),
  production owner publication (changes what the partial-assignment fault
  child tests, needs an integrator decision on that seam), and arming save
  after release (encode before release drops the attempt, or add a post-release
  hook). Note: CMake picked a Homebrew Python for `check_style` in that lane's
  build dir.
- **B, normal Save and Load menus: done, not yet merged** (lane commits
  `dde1a40d6` SaveLoad namespace style fix, `e9a4d5441`, `d7f7ece32`). The
  Save menu writes the selected or first free `saveNNN.ec6` into the native
  MM6 profile through the slot policy and atomic writer, once a load route has
  armed the session. A refused or failed write keeps the menu open with a
  status message and leaves the old slot byte-identical. The Load list reads
  only the native MM6 profile and hides malformed, backup-only, other-map and
  foreign files. In-game load is refused, and main-menu load needs a load
  route. New MM6 files: `MM6FirstMapManualSave.*`, `src/GUI/UI/MM6SaveLoadMenu.*`,
  slot policy additions. New gate section: seed, v4, Save menu child, Load
  menu child, each a fresh process. Tests: unit 1011/1011, both new children
  1/1, MM7 save/load controls 17/17, revert check done after commit. Under
  load the gate needed 1500 s per child. Hooks needed from Lane A: an attempt
  that accepts numbered slots, a production publication entry for `loadGame()`,
  and a point to arm saving after post-world release. Limit: bounded play and
  the save run in a child that reloaded the v4 slot, because production New
  Game is still closed.
- **Merged in one pass, verification build pending:** T (`cf17a2680`..
  `1bbed7ce6`), S (`d52bd57e2`..`4f7aa2a11`), I (`fe6a666f6`..`7b9248970`),
  H (`0a545307b`..`e0252b962`), Codex C2 (`d251707cb`) and C1 (`21b0c04e8`,
  `363d5eee3`), all cherry-picked cleanly. The paperdoll child joined the
  gate runner (`5198aedbb`). The T and H worktrees were retired.
- **P, transition prerequisites: started** in the old S worktree on branch
  `mm6-f4d-transition-prereqs`: MM6 archive encoder, copy-safe rest and heal,
  hireling identity proof, day rollover check.
- **M, shops and guilds: started** in the old I worktree on branch
  `mm6-f4d-shops`: prices, shop screens, stock and refresh, guild spellbooks.
- **Codex now owns** the shared combat files (`Character.cpp`,
  `CastSpellInfo.cpp`, `Monsters.cpp`, `Actor.cpp`, `SpriteObject.cpp`,
  `Spells.cpp`) for the MM7 audit combat fixes, so a second MM6 combat lane
  waits until those land.
- The worktree budget script now says 15 (`1172e1197`). The owner told this
  session to keep 10, so lanes stay at 10 or fewer until the owner confirms.

The merge order is now "as lanes complete", resolving shared-file overlaps at
cherry-pick time, because finished lanes shouldn't wait on unfinished ones.

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
- The first Release install linked XQuartz `libGL`. Codex rebuilt the helper
  against Apple's OpenGL framework and reinstalled it at 15:36. `otool -L`
  shows no `/opt/X11` link. Codex also staged a network-denied, source-owned
  Atlas outside the repo at
  `~/enroth-tools/builds/mm7-integration-20261107/`, not installed, pending a
  graphical play and save/reload check (see `agent_comms/live/` notes and
  `agent_comms/handoffs/codex-mm7-*`).
- The MM7 house pause fix landed as `c11c2c1f9` (with `Prs.Pr2956`) at 15:11,
  before the 15:36 reinstall. The pre-fix binary passed `Issues.Issue840`,
  which does not tape the pause.
- Codex also fixed two A2c findings: the spellbook Grandmaster row and the
  zero-base AC tooltip (`f70627f9c`, `f3f08539c`).
- Remaining MM7 audit fixes: see the ranked lists in
  `MM7_AUDIT_RESULTS_2026-11-07.md` (Break Weapon inversion, Divine
  Intervention recovery, lich immunity, Divine Restoration target, archer
  projectile token first).

## Backlog for new lanes

Standing owner instruction: when a lane finishes and is integrated, start the
next ready item below without waiting to be asked. Keep the number of running
lanes within the three build slots, the 10-worktree cap and the account usage
limit. Each new lane gets its own worktree from the current integration head,
the shared lane rules (`enroth-build`, `enroth-capture`, MM6-owned files,
commit as you go) and exact file ownership.

| Item | Ready after | Files it would own |
| --- | --- | --- |
| Wire Lane T's visited-map owners (plan: `MM6_TRANSITION_WIRING_PLAN.md`) into map transitions, border travel and indoor entry/exit, with the MM6 `isMapIndoor` call sites. Add an MM6 preflight before `onMapLeave()` in `UIMSG_OnIndoorEntryExit` (it runs map-leave events before `startMapTransition()` refuses), then enable Codex C1's MM6 Yes button | T, A, B, C1 merged | `Engine.cpp` transition paths, `Outdoor.cpp` `getTravelDestination`, `Game.cpp` travel handlers, MM6 save sections |
| House NPC dialogue and quest topic (if Lane H stops before its step 4) | H merged | MM6 house window, `UIDialogue.cpp`, `NPCTopics.cpp` dispatches |
| MM6 shop stock, prices, buy and sell, guild spellbook stock | I and H merged | new `MM6Shop*` UI and engine files |
| Transport menu and departure | wiring item above, H merged | `Houses/Transport.cpp` MM6 branch |
| Party death and respawn wiring from Codex C2 | C2, H, B, S merged | one `Game.cpp` dispatch |
| Remaining MM6 spells, monster missiles, Magic and Energy damage, turn mode | S merged | MM6 spell and combat files |
| F5a staged graphical opening check | F4b gate passes with Lane B's normal Save/Load section | none, capture and report only |
| MM7 audit combat fixes: Break Weapon inversion, Divine Intervention recovery, lich immunity, Divine Restoration target, archer projectile token, monster ability slots | S merged, coordinate with Codex in `agent_comms/` first | `Character.cpp`, `CastSpellInfo.cpp`, `Monsters.cpp`, `Actor.cpp` MM7 paths |
| MM7 save robustness: atomic writes via Codex's helper, in-game error on a bad save, Atlas exit-status check | B merged, coordinate with Codex | `SaveLoad.cpp`, `UISaveLoad.cpp`, `launcher/AtlasApp.swift` |
| MM6 calendar start year 1165 (not 1168) as a title-aware start year passed to time conversion, plus MM6 birth years against their base, and reputation variable 215 with a new party field and save section | A, B merged | `Time.h` callers, `Party.cpp`, MM6 save section |
| MM7 native save validator and root context for Codex's atomic writer (see its 22:59 UTC message) | B merged | Codex, detached first |
| MM6 give-item (opcode 41) via the MM6 item generator and drop-item (opcode 34), plus an MM6 portrait sparkle icon mapping | S merged | new MM6 item generator files, interpreter dispatch |
| MM8: its own data inventory, engine adapter and save checks, per plan §18 "After MM6" | MM6 and MM7 work above finished (owner instruction) | the reserved `mm8-isolated` worktree on `mm8-isolated-runtime` |

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
4. After the lanes that touch shared code merge, coordinate the next MM7
   helper rebuild with Codex, which owns the MM7 Atlas build and cutover.
5. Keep this file and the MM7 audit results current as work lands.
