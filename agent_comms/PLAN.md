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
| W3 | One owner-attended visible session of the current installed candidate on the main display (session S1 below). | Codex runs it when authorized, owner attends | **Passed 2026-11-10** in owner session S1 on the installed app: title music, NPC voice, combat sounds and damage heard by the owner, NPC talk and a fight done. The staged session had covered input, focus, pause, shop, spell and Save/Load. |
| W4 | Every main-quest transition from New Game through the shared opening and the Light branch to its ending is exercised through input by a GameTest against the real data, plus fresh-process Save/Load across the chapter boundaries. | Claude coordinates; Codex takes independent code scopes | Castle opening/rebuild and Light Walls fresh-process checkpoints, scavenger Judge/ship, Light city/Walls, three advisor hand-ins, assassination request/cube hand-in and Xenofex map-leave/hand-in passed on integrated source; nine focused Light controls passed together. Real advisor-item and cube acquisition, Xenofex/Pit combat, retrieved-item ending and later save boundary remain open. Four Overthruster route cases plus two wetsuit/Shoals controls and two Xenofex cases passed 8/8 together on integrated source. The later final-link test proves natural L8-to-L9 topic unlock, no-item refusal and held-item fixture ending; it passed 7/7 with the Overthruster cases. The Walls group-1 negative branch is unreachable in tested local data and is not a success-path blocker. |
| W5 | Candidate replaces the installed MM7 under The Atlas. The old app stays as a restorable backup. The installed copy relaunches and loads a new durable save. | Codex root | **Passed 2026-11-10** in S1: the installed app saved a new slot, relaunched from The Atlas and loaded it with party, position and inventory intact. Existing saves unchanged. Only the ini, new logs and three cached tile images changed. |
| W6 | Defects found in W1-W5 that block a player are fixed and their scenario rerun. Non-blocking finds go to the stage 3 audit queue. | Whichever lane is free | Ongoing |

W4 scope, decided 2026-11-10: the owner's gate says "an ending", so stage 1
covers the shared opening and one branch (Light) to its ending. The Dark
branch, both-ending parity and optional regions move to the stage 3 audit.
Open owner decision: whether the ending also needs a human playthrough, or
whether the test chain plus W3 and visible spot checks at chapter boundaries
is enough. Until the owner decides, plan for the test chain and spot checks.

### Owner session S1 — closes W3, W5 and the recorder's first run

Ran 2026-11-10 21:15-21:22 local. W3 and W5 passed. Recorder: logs and save
snapshots were collected in all three launch sessions, but no video, because
macOS Screen Recording permission was not granted to the Atlas. The owner
does not want continuous video capture, and the installed build records
continuously once permission is granted. Recording stays off and permission
ungranted until an on-request replay build is reviewed and installed. A
rolling 60-second replay exists only as uncommitted Codex launcher edits.
Sound IDs 1103, 1503, 1523, 1543 and 1683 logged empty-name load failures
and one `null` sprite lookup failed. A read-only check of the MM7 tables and
archives found all harmless: the sound rows exist with blank names and no
file exists for them (monster idle slots and one map-load decoration sound),
and the `null` frame is the placeholder the seasons option uses to hide four
flowers in winter. Stage 3 keeps two small items: an untraced `sound id 2
not found`, and logging blank-name rows once instead of on every request.
A visible replay of an upstream test trace with the installed upstream helper
and the owner's MM7 data opened an OpenGL window and finished cleanly on
2026-11-10. Its replay mode keeps config in memory, so window placement
on the second monitor was not exercised.
Also for stage 3: the Walls of Mist exit stayed closed once in about 16
fresh chain runs after all three keys were looted. C4 now retries pedestal
clicks until each key is consumed. Find the cause.
Also for stage 3: the Wine Cellar's named vampire loads `Disabled` with the
undocumented attribute `0x10000`. Check whether the original activates it.

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

### W4 continuous chain (proposal, 2026-11-10)

The F1 seams are closed (`codex/mm7-light-npc-persistence` and
`codex/mm7-light-pit-xenofex-seam`, both reviewed). Each chapter now works
from its own setup and its hand-off survives a fresh process. The last W4
piece is one continuous run where every chapter starts from the previous
chapter's real save.

**Harness (one lane, holds `GameTestMain.cpp`).**
- Replace the growing exact-filter list with one table of chapter tests.
- Add a chain mode to the marked disposable profile. A chain chapter starts
  only when exactly the previous chapter's slot is present, loads it through
  the normal Load menu, plays, and saves to its own numbered slot. Chapter 0
  starts from New Game. Every other gate rule stays: marked owner-only temp
  profile, exact filters, fail closed.
- `tools/run_mm7_campaign_chain.py` creates the profile, runs the chapters in
  order one process each, stops at the first failure and reports per-chapter
  results. It keeps every chapter's slot. A chapter starts only when its
  predecessor's slot is present and no later chapter's slot exists. Kept
  slots let a failed chapter rerun from its predecessor and serve the visible
  spot checks at chapter boundaries. The profile stays outside Git, and the
  runner removes it only when asked.

**Chapters (one or two lanes, after the harness).** Each reuses the existing
chapter helpers and closes with tapes that assert the next chapter's entry
state.

| Chapter | From | To |
| --- | --- | --- |
| C0 | New Game | Scavenger hand-ins and ship to Harmondale |
| C1 | Harmondale arrival | Castle goblins cleared and the butler's repair request |
| C2 | Repair request | Stone City rescue and castle rebuilt |
| C3 | Castle rebuilt | Messenger after the long rest, judge, Light chosen |
| C4 | Light chosen | Celeste, Walls hand-in |
| C5 | Walls hand-in | Three adviser hand-ins and the Tolberti cube hand-in |
| C6 | Xenofex request | Xenofex hand-in and the final task |
| C7 | Final task | Overthruster retrieved and the Light ending |

Rules for chain chapters. No quest bit, NPC field or map writes at a chapter
start, since all of it must come from the loaded save. Item acquisition and
kills may stay fixtures where no acquisition test exists yet, each named in
the chapter's first comment and listed in the handoff. Party strength gets
one documented fixture in C0 instead of per-test writes. Teleports between
maps are allowed for now and listed, because real travel is stage 3 breadth.

Order: the harness lane first. Then C0-C3 and C4-C7 as two lanes, which only
share the harness table.

State on 2026-11-10 late evening: the harness with its review fixes (pass
markers, refusal of a chapter's own or later slot, Margaret's tour declined
by input, unimplemented chapters refused before a profile is touched), the
real Xenofex fight and the Light quest-item acquisition tests are on
`master` at `56b5968ab`. The full headless MM7 suite passed 538 with two
skipped, C0 passed through the runner, and the five legacy writer and
reader pairs passed. Chapters C1-C3 are in a Claude lane. C4-C7 follow on
top of them.

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
3. Actor- and character-target scrolls (Fire Bolt and similar) still lose the scroll when targeting is cancelled. Fixed on `claude/mm7-actor-scroll-cancel` (`9ed8a074a`, `1fba84a61`), reviewed, waiting for integration. Other targeting exits still use the scroll up, which goes to the stage 3 queue.
4. `master` failed full `check_style` with 75 errors in about 55 files, mostly MM6. Fixed on `claude/master-style-fix` at `349e55d62`, reviewed, waiting for integration.

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
| M1 | Done on `claude/mm6-new-game`, waiting for integration (gate 27/27). Production New Game with no test override or test anchor: party creation by input, first map, Save, fresh-process Load, move, Save with backup rotation. Children of `tools/run_mm6_firstmap_gate.py`. Headless. | 1-2 weeks |
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

Start S-NG with S-SWEEP, then S-BORDER with S-BOOKS. The reputation
executor is small and independent of all four and can fill any free slot. S-NG and S-BORDER both
touch shared core, so they merge one at a time with M7 between.

Risk probe results (read-only, 2026-11-10, throw sites spot-checked by
Claude). The detailed map is Git-ignored under `local-game-data/mm6/`. The
main quest spans 26 maps plus the global script.

- **Reputation variable (215) has no MM6 executor.** 24 main-quest event
  records throw on it, covering every council reward and the traitor step.
  Smallest fix with the biggest payoff, so it becomes the first M4 scope.
- **Monster combat is the largest M4 item.** Every MM6 monster missile launch
  throws, and ranged monsters stand on 21 of the 26 maps. 12 unverified
  monster spells appear on the path. Poison, magic and energy attack labels
  and most special attacks throw on hit. The first map is clean.
- **Event spells and damage.** Event CastSpell always throws for MM6, with six
  spell IDs on the path. Most event ReceiveDamage kinds throw. Three
  unconditional timers throw shortly after entering their dungeons.
- **Party spells.** Flight (21) and water walking (27) look required, one for
  a plateau house and one for an island dungeon whose only boat is gated
  behind a later step. Flight also needs the event cast that punishes flying
  on some outdoor maps. A throwaway test at the plateau edge should confirm.
- **Travel.** Most of the path is reachable on foot from the first map. Four
  boat routes are required, one of them a quest-gated special route. Three
  dungeons are entered through house exits with quest-bit restrictions,
  which the MM6 house screen does not support yet.
- **NPC topics route correctly.** One quest NPC sits in a house no script
  enters, and one scripted NPC conversation uses a dialogue window with no
  MM6 branch. Both need a runtime probe.
- **Engine rules with no script writer:** three quest bits the original
  executable sets (a stable reward, the finale gate, a reactor door) need
  traces from the executable.
- **Not a problem:** event opcodes 5, 20, 27 and 28 on the main quest, and
  opcode 26 questions.

Runtime confirmation (throwaway probe, 2026-11-10, nothing committed). The
three dungeons with unconditional script-cast timers all throw on schedule:
Castle Alamos (`cd1`), the Free Haven Sewer (`sewer`) and the Control Center
(`sci-fi`), at about 20, 10 and 3 seconds after entering. Two load findings
for M2: the sewer only loads with actors off, because generated NPCs
overflow the pool. And a transition from the first map into a map with
chests refuses, because the previous map's chest positions are still loaded.

Map sweep (S-SWEEP, `claude/mm6-map-sweep`, 2026-11-10, all 67 maps, one
process each, results Git-ignored). 58 maps load and 32 finish every phase
with actors. Of the 26 main-quest maps 22 load and 10 finish clean. First
throws ranked by main-quest maps blocked:
1. A hard crash on 9 main-quest maps (17 in all, including the first map):
   an award event plays the party buff animation, its icon name is missing
   from the MM6 icon table, and `IconFrameTable::animationLength` reads
   index -1. Next lane.
2. Generated NPC pool overflow at load on 4 main-quest maps (6 in all).
3. Event CastSpell on 3 main-quest maps by timer (15 maps with faces).
4. Unverified damage channel on 3 main-quest maps.
5. An `Indoor.cpp` render assert when loading 3 non-quest maps at sector 0.
714 face or decoration references point at missing event IDs and only warn.
Missing assets are limited to a few sounds. Monster missiles and spells are
under-sampled because most maps tick only a few frames.

Revised M4 build order: the award-animation crash, reputation executor
(done on `claude/mm6-reputation-var`, display and other producers still
open), event CastSpell and damage
kinds starting with the three timers, monster missiles and spells, house
exits and boat routes on top of M2, flight and water walking, the MM6 finale,
then the executable-only quest rules. S-SWEEP should also fire each map's
timer and trap events once.

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
- Keep at least 10 GB free on disk. The shared ccache is capped at 3 GB
  (2026-11-10). Each new worktree build adds about 1.5-2 GB, so check
  `df -h /` before starting one. Codex freed 2.3 GiB of merged build
  output on 2026-11-10, leaving 14 GiB free.
- Integrate reviewed commits into `master` one at a time. Codex root
  integrates, Claude reviews lane output before it goes in.
- Game data, saves, captures and detailed traces stay out of Git.
