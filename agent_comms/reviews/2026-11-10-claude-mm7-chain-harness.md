# Review — `codex/mm7-chain-harness` (`a30db1654`)

- Reviewer: Claude Code (lead), via a read-only review agent. No build or run.
- Verdict: safe to integrate as far as safety goes. Fix F1, F2, F4 and F5 and run the missing negatives before any C1 chapter lands on it.

## Safety, all clear

The gate canonicalises the profile, requires the system temp parent, the `mm7-save-gate-` prefix, owner-only permissions and the exact marker, so the owner's `MM7-Atlas` profile cannot pass. Nothing writes to the data path. It fails closed on a wrong or compound filter, a gate filter without a profile, a missing predecessor, a later slot, symlinks, subdirectories, extra files and leftover atomic temp files. Ordinary GameTests are unchanged, and `Run_GameTest_Headless_Parallel` never selects the disabled chain tests. The code keeps every slot, as the plan now asks.

## Should fix

- **F1. Runner resume trusts slot presence as a pass** (`tools/run_mm7_campaign_chain.py`, `completed_chapters()`). A chapter that saves and then fails is skipped on resume, and `--through N` can exit 0 on it. Record a per-chapter pass marker written only after a clean exit and trust only marked slots, or require every chapter to save as its last statement.
- **F2. A chapter may start with its own slot present** (`GameTestMain.cpp`, the `chapter <= gate.chapter` loop). A stale own slot can be loaded or rotated to `.bak`. Reject it (`< gate.chapter`) and rerun through a fresh profile copy, or have every chapter `ASSERT_FALSE` its own slot as C0 does.
- **F4. The claim file still states the old delete-older-slots policy** and promises checks the handoff does not report.
- **F5. `NoMargaret` writes `QBIT_EMERALD_ISLAND_MARGARET_TOUR_OFF`** through the New Game debug path. That bit is a quest-state write that rides in every later chapter's save. Decline the tour through input, or name the bit as a fixture in the test and handoff.

## Not run yet

The C1-without-C0 negative, a later-slot negative, the legacy writer and reader pairs after the gate table refactor, `check_style`, unit tests and the headless parallel suite, and the runner's `--profile` resume and `--remove-profile` paths.

## Nits

The runner has no subprocess timeout and creates the profile before validating paths. A failed C0 needs a fresh profile and the runner should say so. The chapter count is hard-coded in three places. C0 duplicates the scavenger delivery table instead of reusing it, its `House_Npc` and `Option` presses lost the who/what comments, and it checks only the last map instead of the whole tape. Its closing tapes do not show C1's starting bits still clear. The commit message has no reasoning paragraph. `Mm7ProfileGate` has no doxygen block and `preserveSave` duplicates `!writer`.
