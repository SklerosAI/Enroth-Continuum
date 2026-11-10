# MM7 Emerald Island scavenger handoff — 2026-11-10

- Branch: `codex/mm7-scavenger-path`
- Coordination claim: `3793f77ae`
- Test code: `46d846b4f`
- Files owned: new `test/Bin/GameTest/GameTests_MM7ScavengerPath.cpp` and its `CMakeLists.txt` registration.
- Scope: six Judge hand-ins using held contest-item fixtures and UI dialogue, all-six versus wealthy-hat-missing branches, Markham ship offer and departure on the positive branch. No quest outcome bits are pre-set.

The first focused run reached and verified all five negative-branch hand-ins and all six positive-branch hand-ins. It stopped on the win assertion immediately after the sixth delivery. Local event inspection showed the Judge checks completion on a follow-up dialogue action, so the test now performs that action in both branches before checking the win bit. That earlier failure is the revert-and-fail check for the follow-up action.

Under renewed headless-only authorization, the corrected focused `MM7ScavengerPath.JudgeAndShip` passed 1/1. It verified the hat-missing branch remains on Emerald Island without win, ship or escape bits; the all-six branch gains the win after the follow-up Judge action, Markham offers the ship, and the party arrives in Harmondale. The existing `Issues.Issue201:Issues.Issue2908` controls passed 2/2. No graphical app was launched. Static checks: `OpenEnroth_GameTest` built; `check_style_OpenEnroth_GameTest` and `git diff --check` passed.

Focused rerun command, with the private MM7 path supplied locally:

```sh
build/test/Bin/GameTest/OpenEnroth_GameTest --game mm7 --headless --data-path "$MM7_DATA_PATH" --test-path test/Data --gtest_filter=MM7ScavengerPath.JudgeAndShip --gtest_brief=1 --log-level=error
```

The focused and control gates have passed; no further rerun is required for this delivery slice.

Boundary: this covers delivery and ship dialogue, not the six item acquisition routes. Held-item fixtures isolate the Judge's event; one source-backed chest pickup and any trade routes need their own input checks before O2 can be called a full playthrough. A fresh-process save/load after departure is also a separate W4 checkpoint. No private game files, raw scripts, saves, asset inventories, or local logs were committed.
