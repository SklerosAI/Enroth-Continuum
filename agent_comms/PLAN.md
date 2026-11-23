# Delivery plan — title-owned execution

Codex is the sole arbiter and delivery owner for MM7. Claude Code is the sole
arbiter and delivery owner for MM6. Each assigns, accepts and integrates its
own title's work on its own branch, `codex/mm7` and `claude/mm6`. `master` is
neutral and carries neither title's latest work. Use the matching `codex/` or
`claude/` prefix for lane branches. `CURRENT_WORK.md` holds the board, and the
delivery order itself is in `.llm-wiki/Current-Delivery-Order.md`.

Stages run in order: (1) MM7 working, (2) MM6 working with MM7 preserved,
(3) full MM7 audit, (4) full MM6 audit. A working gate and an audit are
separate. A stage advances only when every row of its gate has the stated
evidence. Headless tests do not stand in for a visible check of the current
candidate.

## Stage 1 — MM7 working gate (closed 2026-11-11)

| Row | Evidence that closes it | State |
| --- | --- | --- |
| W1 | Current integrated source builds into a staged app. Title lock and signature hold. A headless fresh-process Save/quit/relaunch/Load passes on that build in an isolated profile. | Passed |
| W2 | The Harmondale raid test runs and passes. | Passed |
| W3 | One owner-attended visible session of the installed candidate on the main display: title music, NPC voice, combat sound, NPC talk and a fight. | Passed in owner session S1, 2026-11-10 |
| W4 | Every main-quest transition from New Game to an ending is exercised through input by a GameTest against the real data, with fresh-process Save/Load across the chapter boundaries. | Passed. The Light chain C0-C7 and the Dark chain C0-C7 each run through real saves from New Game to their ending. |
| W5 | The candidate replaces the installed MM7 under The Atlas. The installed copy relaunches and loads a new durable save. | Passed in S1 |
| W6 | Defects found in W1-W5 that block a player are fixed and their scenario rerun. Others go to the stage 3 queue. | Passed. No known player-blocking MM7 defect. |

The chain runs through `tools/run_mm7_campaign_chain.py`, one process per
chapter, in a marked temporary profile. A chapter starts only from its
predecessor's slot, writes no quest state at its start and saves to its own
slot. Named item, kill and party-strength fixtures and teleports between maps
are listed in each chapter's first comment.

Whether the current MM7 build is ready for an owner playthrough is tracked in
`.llm-wiki/MM7-Playthrough-Go-No-Go.md`.

## Stage 2 — MM6 working (active)

MM6 work, its gate rows and its status live on `claude/mm6` in `docs/`, start
with `docs/mm6-campaign-status.md` and `docs/mm6-playthrough-go-no-go.md`.
The MM6 rows on this branch are out of date.

MM7 must stay working through stage 2. After every merge that touches a
shared-core file, run `--gtest_filter='MM7*:VendorRefreshIntegration.*'` and
the separate-process MM7 writer and reader pairs. At milestones run the full
headless suite and both campaign chains. Only one lane at a time may hold a
shared-core file: `src/Application/Game.cpp`, `src/Engine/Engine.cpp`,
`src/Engine/SaveLoad.cpp`, `src/Engine/Graphics/Outdoor.cpp`,
`src/Engine/Graphics/Indoor.cpp`, `src/Engine/Evt/EvtInterpreter.cpp`,
`src/GUI/UI/UIHouses.cpp`, `src/GUI/UI/Houses/Transport.cpp`,
`src/GUI/UI/UISaveLoad.cpp`, `src/GUI/UI/UITransition.cpp`,
`src/GUI/UI/NPCTopics.cpp`, `src/GUI/UI/Books/*.cpp`,
`src/Application/GameOver.cpp`, `src/GUI/UI/UIGameOver.cpp`.

## Stages 3 and 4 — audits (queued)

Stage 3 works through `project/MM7_AUDIT_CHECKLIST.md`, the open findings in
`project/MM7_AUDIT_RESULTS_2026-11-07.md` and these items from stage 1:
- An untraced `sound id 2 not found` in the MM7 log.
- Sound rows with blank names log a failure on every request instead of once.
- The Walls of Mist exit once stayed closed after all three keys were looted.
  C4 retries pedestal clicks until each key is consumed. Find the cause.

Stage 4 works through full MM6 campaign coverage on `claude/mm6`. When the
stage 3 audit starts is the owner's call.

The built-in Ctrl+Shift+R MM7 trace recorder is for short repros only. It
reloads a save, switches to deterministic mode and redirects saves to an
in-memory filesystem until the trace stops.

## Limits in force

- No lane, worktree or branch count cap while a round runs. Each round ends
  with one branch and one worktree per title.
  `tools/check_worktree_budget.py` still enforces an older cap, so read its
  counts, not its verdict.
- Keep at least 10 GB free on disk. Check `df -h /System/Volumes/Data`, load
  and swap before build-heavy work. The shared ccache is capped at 2 GB.
- Builds go through `~/enroth-tools/bin/enroth-build`, which allows two
  builds at once across the machine.
- Game data, saves, captures and detailed traces stay out of Git.
