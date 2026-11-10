# Lead plan — MM7 first

Owner: Claude Code, planning and coordination lead since 2026-11-10.
This is a living plan. `CURRENT_WORK.md` holds the lane board. The delivery
order itself is in `.llm-wiki/Current-Delivery-Order.md`.

Stages run in order: (1) MM7 working, (2) MM6 working with MM7 preserved,
(3) full MM7 audit, (4) full MM6 audit. A working gate and an audit are
separate. A stage advances only when every row of its gate has the stated
evidence. Headless tests and the older installed baseline do not stand in for
a visible check of the current candidate.

## Stage 1 — MM7 working gate

| Row | Evidence that closes it | Owner | State |
| --- | --- | --- | --- |
| W1 | Current integrated source builds into a staged app. Title lock and signature hold. A headless fresh-process Save/quit/relaunch/Load passes on that build in an isolated profile. | Codex lane B (`codex/mm7-candidate-validation`) | Passed 2026-11-10 (staged build, Save and fresh-process Load 1/1 each) |
| W2 | The disabled Harmondale raid test runs and passes, with the quest route fixed if needed. | Codex lane A (`codex/mm7-raid-route`) | Passed on integrated source; campaign controls 6/6 |
| W3 | One owner-attended visible session of the current installed candidate on the main display (session S1 below). | Codex runs it when authorized, owner attends | Earlier staged session passed durable Save/Load; installed signed build still needs visible smoke, audio, ordinary NPC and monster fight. Owner currently authorizes headless checks only. |
| W4 | Every main-quest transition from New Game through the shared opening and the Light branch to its ending is exercised through input by a GameTest against the real data, plus fresh-process Save/Load across the chapter boundaries. | Claude coordinates; Codex takes independent code scopes | Castle opening/rebuild and Light Walls fresh-process checkpoints, scavenger Judge/ship, Light city/Walls, three advisor hand-ins, assassination request/cube hand-in and Xenofex map-leave/hand-in passed on integrated source; nine focused Light controls passed together. Real advisor-item and cube acquisition, Xenofex/Pit combat, retrieved-item ending and later save boundary remain open. Four Overthruster route cases plus two wetsuit/Shoals controls and two Xenofex cases passed 8/8 together on integrated source. The later final-link test proves natural L8-to-L9 topic unlock, no-item refusal and held-item fixture ending; it passed 7/7 with the Overthruster cases. The Walls group-1 negative branch is unreachable in tested local data and is not a success-path blocker. |
| W5 | Candidate replaces the installed MM7 under The Atlas. The old app stays as a restorable backup. The installed copy relaunches and loads a new durable save. | Codex root | Signed candidate installed headlessly after owner authorization; prior app backed up, profile and saves untouched. Relaunch/Load acceptance awaits graphical authorization. |
| W6 | Defects found in W1-W5 that block a player are fixed and their scenario rerun. Non-blocking finds go to the stage 3 audit queue. | Whichever lane is free | Ongoing |

W4 scope, decided 2026-11-10: the owner's gate says "an ending", so stage 1
covers the shared opening and one branch (Light) to its ending. The Dark
branch, both-ending parity and optional regions move to the stage 3 audit.
Open owner decision: whether the ending also needs a human playthrough, or
whether the test chain plus W3 and visible spot checks at chapter boundaries
is enough. Until the owner decides, plan for the test chain and spot checks.

### Owner session S1 — closes W3, W5 and the recorder's first run

The staged W3 session already passed launch, opening movie, New Game, input,
focus and pause, a shop purchase, a spell, and named Save then fresh-process
Load. What is left fits in one sitting of about 20 minutes on the main
display, on the installed app with the normal profile, because that is what
W5 accepts.

Before: Codex records the modification times of the normal MM7 profile files
and saves, checks at least 4 GiB free on the recording volume, and takes a
capture of each step with `enroth-capture`.

1. Open the installed Atlas. Turn on Record gameplay with a local
   destination. Grant macOS Screen Recording if asked.
2. Play MM7. Owner confirms the title music is audible.
3. New Game or Load. On Emerald Island talk to a roaming NPC (not the guide)
   and fight one monster to death. Owner confirms combat sound effects.
4. Save to a new named slot. Exit to The Atlas. The recorder stops by itself.
5. Play MM7 again, Load that slot. Party, position and inventory match.
6. Exit and close every window. Codex checks at a high level that the
   recording folder has video, log ranges and a save snapshot, that nothing
   landed in the repository, and which normal profile files changed (only
   the new save, its backup rotation and the ini are expected).

Pass closes W3 and W5. A failure becomes the top of the defect queue.

### W4 campaign path

A read-only mapping (2026-11-10) of the private decoded campaign scripts
against every MM7 GameTest found 37 main-quest transitions across the opening
and both branches. 11 are exercised through input, 3 only by recorded traces
and 23 not at all. The detailed map is Git-ignored under
`local-game-data/mm7/working-gate-2026-11-10/`.

Covered through input: the opening start and ship departure, the castle
timer and arbiter messenger, the judge pick-up and arbiter choice, and the
final hand-in on each branch. Uncovered on every playthrough: the first castle
visit through the rebuild (O5-O10). That stretch has a whole-group kill check
that runs on map exit, the same mechanism the disabled raid test is stuck on.

Stage 1 test scopes, in order. Each is one GameTest sized like `ArbiterChoice`.

1. Castle goblins (O5-O7), all killed versus one survivor. Shares the
   group-kill check with W2, so it starts after lane A's fix lands.
2. Castle rebuild (O8-O10), all story NPCs gathered versus one missing.
3. Emerald Island scavenger hunt (O2), every delivery bit taped.
4. Light city entry and Walls of Mist (L1-L4), no kill versus one kill of the
   protected group, plus the daytime-only throne room.
5. Light advisor tasks and Xenofex (L5, L8).
6. Light assassination task (L6-L7).
7. Overthruster retrieval (L10-L14), including a party member without a
   wetsuit.

Fresh-process Save/Load checkpoints go after scopes 2, 4 and 7. Stage 3
takes the Dark branch scopes (D1-D10) and judge-swap and full-hireling cases.

New campaign-path tests start after lane A integrates, because lane A owns
`GameTests_MM7Campaign.cpp` until then.

### Stage 1 defect queue

Ordered by how badly a player is blocked. A lane takes the top item when free.

1. Any defect found by W1, W3 or W4.
   - W4 seams and NPC topic persistence: **resolved, not a defect.** The
     real Walls hand-in clears the throne NPC's topic before Save, and fresh
     Load keeps every NPC 79-83 house and topic (`codex/mm7-light-npc-persistence`,
     reviewed by Claude). The NPC 80 topic seam after the Tolberti hand-in is
     Codex's active lane.
   - W4 test hardening, review findings F2-F6, after the seams.
   - Continuous chained Light run and real quest-item acquisition, after
     that. Needs a chain mode in the disposable-profile gate.
2. Cancelled item-target spell scroll is consumed anyway (`src/Engine/Objects/Character.cpp`, the open TODO at the scroll cast). Player-facing item loss, not blocking. Fixed on `claude/mm7-scroll-cancel` at `401d8e2a4`, reviewed, waiting for integration.
3. Actor- and character-target scrolls (Fire Bolt and similar) still lose the scroll when targeting is cancelled. Same family, not blocking.
4. `master` fails full `check_style` on files no recent lane touched: missing includes in `src/Engine/Mods/PrivateMods.cpp` and `src/Engine/Objects/Chest.cpp`, a using-directive in `src/Engine/Mods/PrivateMods_ut.cpp`. Small, should go in before more lanes merge.

## Stage 2 — MM6 working (queued)

Drafted 2026-11-10 by a read-only planning pass, with its key source claims
spot-checked by Claude. Source facts that shape it: production MM6 New Game
ends in an uncaught throw right after party creation (`saveNewGame()` in
`src/Engine/SaveLoad.cpp`). `getTravelDestination()` returns nothing for MM6,
and every cross-map path calls `autoSave()`, which throws for MM6. Only one
MM6 player spell (67) is verified. The MM6 finale still runs MM7's game-over
path to Harmondale. `codex/mm6-notes-book` has no commits that are not
already in `master`, so its scope restarts from `master`.

| Row | Evidence that closes it | Size |
| --- | --- | --- |
| M1 | Production New Game with no test override or test anchor: party creation by input, first map, Save, fresh-process Load, move, Save with backup rotation. Children of `tools/run_mm6_firstmap_gate.py`. Headless. | 1-2 weeks |
| M2 | Live travel: border walk out and back, indoor entry and exit, transport. Save on a later outdoor map and an indoor map, fresh-process Load restores chests, dead monsters and doors on the active and the parked map. `autoSave` never throws for MM6. Headless. | 3-6 weeks |
| M3 | Party death and respawn by MM6 rules, in-game Load, quick and autosave work or refuse cleanly. Headless. | 1 week |
| M4 | Main-quest path GameTests from New Game to the MM6 finale, input-driven, with fresh-process checkpoints at chapter boundaries. The finale never enters an MM7 map. Combat-outcome fixtures allowed as in W4. Headless. | 6-12+ weeks, narrowed by the probes |
| M5 | Owner sessions: (a) F5a Load profile with HUD, books, paperdoll and Save/quit/relaunch/Load, (b) the S1-style session on MM6 including travel and a dungeon, (c) spot checks at M4 checkpoints and the ending. Visible. | Owner hours |
| M6 | Atlas install of a sibling MM6 helper with its own profile. Installed MM6 relaunches and loads. MM7's helper, profile and saves unchanged and MM7 still loads. | 3-5 days |
| M7 | MM7 stays working. After every merge touching a shared-core file: `--gtest_filter='MM7*:VendorRefreshIntegration.*'`, the MM7 controls in the first-map gate, and the separate-process writer and reader pairs (W1, castle rebuild, Walls). At milestones the full headless suite. | Continuous |
| M8 | Player-blocking defects from M1-M7 fixed and rerun. Others go to stage 4. | Ongoing |

Critical path: M1, M2, then M4 chapter by chapter, M4 finale, M5b and c, M6.
M5a can run whenever the owner gives display time. M3 runs alongside M2.

Shared-core files that only one lane may hold at a time, each merge followed
by M7: `src/Application/Game.cpp`, `src/Engine/Engine.cpp`,
`src/Engine/SaveLoad.cpp`, `src/Engine/Graphics/Outdoor.cpp`,
`src/Engine/Graphics/Indoor.cpp`, `src/Engine/Evt/EvtInterpreter.cpp`,
`src/GUI/UI/UIHouses.cpp`, `src/GUI/UI/Houses/Transport.cpp`,
`src/GUI/UI/UISaveLoad.cpp`, `src/GUI/UI/UITransition.cpp`,
`src/GUI/UI/NPCTopics.cpp`, `src/GUI/UI/Books/*.cpp`,
`src/Application/GameOver.cpp`, `src/GUI/UI/UIGameOver.cpp`.

First code scopes, two at a time:

1. S-NG, production New Game (M1). `saveNewGame()`'s MM6 branch,
   `src/Engine/MM6/MM6FirstMapManualSave.*`, a new MM6 New Game session
   module, a new `GameTests_MM6NewGame.cpp`, the gate runner. Not `Game.cpp`.
2. S-SWEEP, map, event and monster sweep (test only, DISABLED by default).
   Loads every MM6 map headless, runs one frame and the on-load events, and
   counts throws and missing assets into a Git-ignored folder. No `src/`.
3. S-BORDER, live border travel within a session (M2 part 1). A new MM6 map
   transition module plus hooks in `Outdoor.cpp`, `Game.cpp` and `Engine.cpp`.
4. S-BOOKS, Notes and Map books (M5a prerequisite), with `Issues.Issue2903`
   as the MM7 control.

Start S-NG with S-SWEEP, then S-BORDER with S-BOOKS. S-NG and S-BORDER both
touch shared core, so they merge one at a time with M7 between.

Risks to retire early, read-only where possible: main-quest dependence on
unverified spells (R1), unresolved event opcodes and variables on main-quest
maps (R2), later maps only loaded in fixtures (R3, S-SWEEP), save coverage
past the first map (R4), unverified monster spells on main-quest maps (R5),
NPC topic misrouting (R6), whether every main-quest map is reachable on foot
or by required boat (R7), MM7's ending path (R8, confirmed), and missing shop,
guild and trainer services (R9).

## Stages 3 and 4 — audits (queued)

Stage 3 works through `project/MM7_AUDIT_CHECKLIST.md` plus the stage 1
defect queue's non-blocking items. Stage 4 works through
`project/MM6_IMPLEMENTATION_CHECKLIST.md` and full campaign coverage.
For the owner's endgame playtest, a local passive Bash collector and Atlas
recording control are staged after synthetic script and headless bundle checks.
The first actual video/launcher recording and external-drive check remain open
under the owner's current headless-only authorization. Keep all output outside
Git. The built-in Ctrl+Shift+R MM7 trace recorder is
useful for short repros only: it reloads a save, changes to deterministic mode
and redirects saves to an in-memory filesystem until the trace stops.

## Limits in force

- Two code-changing lanes at most while both Codex lanes are active. Claude
  takes a code lane only when one frees.
- Ten registered worktrees at most. Reuse a merged worktree before creating one.
- Keep at least 10 GB free on disk. Codex freed 2.3 GiB of merged build
  output on 2026-11-10, leaving 14 GiB free.
- Integrate reviewed commits into `master` one at a time. Codex root
  integrates, Claude reviews lane output before it goes in.
- Game data, saves, captures and detailed traces stay out of Git.
