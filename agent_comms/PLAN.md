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
| W3 | One owner-attended visible session of the staged candidate on the main display (checklist below). | Lane B runs it, Claude drafts the checklist, owner attends | Partial staged session passed durable Save/Load; final signed build still needs visible smoke, audio, ordinary NPC and monster fight. Owner currently authorizes headless checks only. |
| W4 | Every main-quest transition from New Game through the shared opening and the Light branch to its ending is exercised through input by a GameTest against the real data, plus fresh-process Save/Load across the chapter boundaries. | Claude coordinates; Codex takes independent code scopes | Castle opening/rebuild, fresh-process rebuild checkpoint and scavenger Judge/ship tests passed on integrated source. Light branch and remaining save boundaries open. |
| W5 | Candidate replaces the installed MM7 under The Atlas. The old app and normal profile stay as a restorable backup. The installed copy relaunches and loads an existing save. | Lane B | Waiting on W3 |
| W6 | Defects found in W1-W5 that block a player are fixed and their scenario rerun. Non-blocking finds go to the stage 3 audit queue. | Whichever lane is free | Ongoing |

W4 scope, decided 2026-11-10: the owner's gate says "an ending", so stage 1
covers the shared opening and one branch (Light) to its ending. The Dark
branch, both-ending parity and optional regions move to the stage 3 audit.
Open owner decision: whether the ending also needs a human playthrough, or
whether the test chain plus W3 and visible spot checks at chapter boundaries
is enough. Until the owner decides, plan for the test chain and spot checks.

### W3 visible session checklist

One launch, batched, so the owner gives display time once.

1. Launch the staged candidate from the staged Atlas. Title screen draws and the opening movie plays with sound.
2. New Game, party creation with keyboard and trackpad. Enter Emerald Island.
3. Movement with keyboard, turning with mouse. Talk to an NPC, enter a shop and buy something, cast a spell, fight one monster.
4. Music and sound effects audible (owner confirms, a device opening is not proof).
5. Window focus loss and return, pause and resume.
6. Save to a new slot through the menu. Quit to The Atlas.
7. Relaunch, Load that slot. Position, inventory, gold and the bought item match.
8. Load one of the owner's existing saves from the installed baseline profile copy.
9. One hidden-capture or screen grab per step via `enroth-capture`, kept out of Git.

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
2. Cancelled item-target spell scroll is consumed anyway (`src/Engine/Objects/Character.cpp`, the open TODO at the scroll cast). Player-facing item loss, not blocking. Found by Codex read-only triage.

## Stage 2 — MM6 working (queued)

Resume from `project/MM6_RESUME_HANDOFF.md` and the retained
`codex/mm6-notes-book` worktree. Order: Notes and Map books, visible F5a Load
and input check, production New Game, live travel, party death, then campaign
progression and endings. Every shared-core change reruns the MM7 controls and
the W1 save/reload check so stage 1 stays closed.

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
