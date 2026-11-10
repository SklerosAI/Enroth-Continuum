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
| W1 | Current integrated source builds into a staged app. Title lock and signature hold. A headless fresh-process Save/quit/relaunch/Load passes on that build in an isolated profile. | Codex lane B (`codex/mm7-candidate-validation`) | Active |
| W2 | The disabled Harmondale raid test runs and passes, with the quest route fixed if needed. | Codex lane A (`codex/mm7-raid-route`) | Active |
| W3 | One owner-attended visible session of the staged candidate on the main display (checklist below). | Lane B runs it, Claude drafts the checklist, owner attends | Waiting on W1 |
| W4 | Every main-quest transition from New Game through both endings is exercised through input by a GameTest against the real data, plus fresh-process Save/Load across the chapter boundaries. | Claude, next free code lane | Mapping in progress (read-only) |
| W5 | Candidate replaces the installed MM7 under The Atlas. The old app and normal profile stay as a restorable backup. The installed copy relaunches and loads an existing save. | Lane B | Waiting on W3 |
| W6 | Defects found in W1-W5 that block a player are fixed and their scenario rerun. Non-blocking finds go to the stage 3 audit queue. | Whichever lane is free | Ongoing |

Open owner decision for W4: whether "progression through an ending" also
needs a human playthrough, or whether the input-driven test chain plus the W3
visible session and visible spot checks at chapter boundaries is enough.
Until the owner decides, plan for the test chain and the spot checks.

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

A read-only mapping of the main-quest transitions against existing tests is
running. Its public-safe result (quest-bit and map enum names only) will be
appended here. Known so far: the arbiter choice, the messenger timing and both
ending hand-ins are input-tested in `GameTests_MM7Campaign.cpp`. The opening
through the castle rebuild and the two act-two task chains are not.

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

## Limits in force

- Two code-changing lanes at most while both Codex lanes are active. Claude
  takes a code lane only when one frees.
- Ten registered worktrees at most. Reuse a merged worktree before creating one.
- Keep at least 10 GB free on disk. On 2026-11-10 at 09:55 local there was
  12 GiB free with load near 13, so build-heavy work needs space freed first.
- Integrate reviewed commits into `master` one at a time. Codex root
  integrates, Claude reviews lane output before it goes in.
- Game data, saves, captures and detailed traces stay out of Git.
