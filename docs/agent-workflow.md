# Agent workflow

This page is how Codex, Claude and their subagents share this repository: who owns what, how a lane starts, where it
records its work, and how it gets merged.

## Agents and ownership

Two agents work here, each with the final word on one title. Neither redirects, accepts or changes the other's title
without a new instruction from the owner.

| Agent | Owns | Rule set |
| --- | --- | --- |
| Codex | MM7: the working path, the audit, MM7 defects, MM7 lanes and their integration onto `master`. Codex leads overall. | [AGENTS.md](../AGENTS.md) |
| Claude Code | All MM6 work: planning MM6 lanes, reviewing them and merging them into `claude/mm6`. | [.claude/CLAUDE.md](../.claude/CLAUDE.md) and [HACKING.md](../HACKING.md) |
| Subagents | One bounded lane each, given by a parent agent. A subagent doesn't widen its task or touch another lane's files. | The parent's rule set |

The overall order of work is in the [project overview](project-overview.md). MM8 has no runtime and no lanes.

Code that both titles run is shared core. The owner released shared MM6 and MM7 code to Claude while Codex is idle,
so an MM6 fix may change a shared file when it has to. Check the live mailbox for Codex activity before relying on
that. Any change to shared core runs the MM7 controls before it merges, see
[Checks before merging](#checks-before-merging).

Codex learns what Claude did only through commits, files and the live mailbox, and the same holds the other way.
There is no direct channel between the two sessions.

## Lanes and worktrees

A lane is one task in one worktree on one branch. Two agents never write to the same working tree. The main checkout
stays on `master` and is not used for lane edits.

| Item | Convention |
| --- | --- |
| Claude lane branch | `claude/mm6-<lane>`, branched off `claude/mm6` |
| Claude integration branch | `claude/mm6`, in its own worktree |
| Codex lane branch | `codex/<topic>`, usually `codex/mm7-<topic>`. Retired ones sit under `codex/archive/`. |
| Claude worktrees | `../enroth-worktrees/<name>` next to the main checkout |
| Codex worktrees | Codex's managed folder under `~/.codex/worktrees/` |

Worktree folder names are not a contract. A finished worktree is reused for the next lane by checking out a new
branch in it, so a folder name often doesn't match its branch. `git worktree list` shows what is where.

### Budget

Each title owner gets five linked worktrees. The main checkout on `master` counts against neither.
[tools/check_worktree_budget.py](../tools/check_worktree_budget.py) assigns each worktree by its `claude/` or `codex/`
branch prefix, prints both counts and any worktree without a prefix, and exits non-zero when the given agent is at
the cap. Run it before adding a worktree:

```bash
python3 tools/check_worktree_budget.py --agent claude
```

Disk is the other limit, and the owner may trade a sixth worktree for it. Keep at least 10 GB free on the system disk.
A new worktree build costs about 1.5 to 2 GB, so check `df -h /` before starting one. Under load, run game tests one
process at a time, and check `uptime` and `sysctl vm.swapusage` before adding a lane. A finished lane's worktree is
retired or reused at the end of the session, never kept for its own sake. Before removing one, save its uncommitted
changes, local-only commits and ignored private files, and keep its branch ref.

### Starting a lane

1. Check the budget and the disk as above.
2. Create the worktree, or reuse a finished one:
   ```bash
   git worktree add ../enroth-worktrees/<name> -b claude/mm6-<lane> claude/mm6
   ```
3. Build and set up the data paths as [build and dev setup](build-and-dev-setup.md) and [testing](testing.md) describe.
   The worktree's own `local-game-data/` is empty. Point the data variables at the main checkout's copy.
4. Read the docs page for the subsystem, then the source. The docs explain what the code can't say.
5. Work, test, take the two captures, commit, and report to the integrator.

A subagent gets everything it needs in its prompt: the worktree, the branch, the scope, the data paths and the build
and test commands. It has no other context.

## Branches and merging

Since 2026-11-18, MM6 lanes branch off `claude/mm6` and merge back into it. `claude/mm6` collects MM6 work until the
next release tag, and `master` gets no MM6 work until that tag is decided. The [releases page](releases.md) has the
tags.

- Nobody pushes a lane to `master` directly. An integrator merges reviewed lanes, one at a time.
- Claude integrates MM6 lanes into `claude/mm6` with a merge commit, `Merge MM6 <lane> lane`.
- Codex integrates MM7 lanes onto `master`.
- A lane that falls behind its base branch rebases onto it. `master` is never merged into an MM6 branch.
- Commits are never amended and pushed history is never rewritten without an explicit ask. Fixes go on top.

Codex may commit in the main checkout at the same time as a Claude session. Expect a brief `.git/index.lock` clash
and re-read a file before editing it.

## Ownership without claim files

Lanes used to claim files in tracked claim files. Each claim named the owner, the branch and worktree,
the files owned, the purpose and the planned checks, so two lanes never edited the same file. Those files and the
tracked handoff, message and review folders are gone.

Ownership now rides on two things:

- The lane branch name says who owns the lane and what it's for. `git worktree list` and `git branch` show every
  active lane.
- The live mailbox carries anything the other agent has to know now, such as a shared-core file a lane is about to
  change.

When two lanes need the same file, the integrator merges one first and the other rebases onto it.

## The live mailbox

`agent_comms/live/` is a local mailbox for agents on this Mac. Its [README](../agent_comms/live/README.md) is the
protocol, and it's the only tracked file there. Git ignores everything else in it.

- Use the main checkout's `agent_comms/live/`, from any worktree. A worktree's own copy is a different, empty
  folder. The main checkout is the first entry in `git worktree list`.
- Write a new file, never an existing one, named
  `YYYY-MM-DDTHH-MM-SS-sss--from-<sender>--to-<recipient>--<topic>.md`.
- Start the body with `From:`, `To:`, `Topic:` and `Needs reply:` lines under a `#` subject.
- Write to a temporary name and rename it when done, so the reader never sees a partial message.
- Reply in a separate file whose topic begins `reply-`.
- Poll the folder at safe checkpoints.

Never put proprietary game data, passwords, personal data, raw logs, saves, screenshots or asset inventories in it.
A message grants no file ownership. Before a session ends, an unresolved live instruction moves to where it lasts: a
commit message, a docs page, or [open questions](open-questions.md).

## Where work is recorded

There are no per-lane handoff files. Each piece of a lane's record has one home.

| What | Where |
| --- | --- |
| Why a change is what it is | The commit message, a paragraph on what was wrong and why this is the fix |
| A durable fact the code can't say | The `docs/` page for that subsystem, in the same lane |
| Evidence gathered along the way | The lane's report to the integrator, or a comment on the pull request |
| Latest MM6 status and lane results | [mm6-campaign-status.md](mm6-campaign-status.md) |
| Open questions and known bugs | [open-questions.md](open-questions.md) |

A durable fact is something the next engineer needs and can't get from the code: an `MM6.exe` address and what it
does, how the original game behaves, why a rule is what it is. Test counts and dated progress notes are not durable.
Private evidence stays under `local-game-data/`, see [source and privacy](source-and-privacy.md).

## Checks before merging

Each lane follows the testing rules in its rule set. For Claude that means the style checks, the unit tests and the
game tests from [.claude/CLAUDE.md](../.claude/CLAUDE.md), and a game test that fails without the fix. The MM6 game
tests run with `--game mm6` and the filter `MM6*`.

A lane that touches shared core also runs the MM7 controls:

```text
--gtest_filter='Prs.Pr2354:Prs.Pr2956:Issues.Issue840'
```

plus every MM7 test that covers the shared code it changed. The [testing page](testing.md) has the gate runners and
the full suite.

## Captures

Every MM6 task takes two hidden captures, one before the fix and one after, of the screen where the change shows.
They go into the main checkout's `local-game-data/mm6/task-captures-<date>/<lane>/` as `before.png` and `after.png`.
A temporary `DISABLED_` capture test run with `--offscreen-gl` takes them, and it's deleted before the commit. The
[testing page](testing.md) covers offscreen capture.

Inspect the whole frame, not only the part under test. Check the HUD, the portraits, the side panels and the status
bar for misalignment, placeholders and stray art, and say what you saw in the report. A defect that shows in every
capture and goes unremarked reaches the owner first. Captures never go into Git.

## Reviews

A review names the revision it read, the files inspected and the tests it considered. Findings come ordered by
severity, each with a follow-up owner. A clean review says what was checked, not just that it passed. Review findings
go back to the lane owner, and fixes land as new commits on the lane.

## Source

- [tools/check_worktree_budget.py](../tools/check_worktree_budget.py)
- [agent_comms/live/README.md](../agent_comms/live/README.md)
- [.gitignore](../.gitignore)
- [.claude/CLAUDE.md](../.claude/CLAUDE.md)
- [HACKING.md](../HACKING.md)
- [AGENTS.md](../AGENTS.md)
- [test/Bin/GameTest/GameTestOptions.cpp](../test/Bin/GameTest/GameTestOptions.cpp)

See also: [Project overview](project-overview.md), [MM6 campaign status](mm6-campaign-status.md),
[Open questions](open-questions.md), [Testing](testing.md), [Build and dev setup](build-and-dev-setup.md),
[Source and privacy](source-and-privacy.md)
