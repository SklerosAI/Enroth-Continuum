# MM7 Emerald Island scavenger handoff — 2026-11-10

- Branch: `codex/mm7-scavenger-path`
- Coordination claim: `3793f77ae`
- Test code: `46d846b4f`
- Files owned: new `test/Bin/GameTest/GameTests_MM7ScavengerPath.cpp` and its `CMakeLists.txt` registration.
- Scope: six Judge hand-ins using held contest-item fixtures and UI dialogue, all-six versus wealthy-hat-missing branches, Markham ship offer and departure on the positive branch. No quest outcome bits are pre-set.

The first focused run reached and verified all five negative-branch hand-ins and all six positive-branch hand-ins. It stopped on the win assertion immediately after the sixth delivery. Local event inspection showed the Judge checks completion on a follow-up dialogue action, so the test now performs that action in both branches before checking the win bit. The corrected test has **not** been rerun because the owner's time-limited headless authorization ended. The Markham and departure segment in this test is therefore also unverified; its input sequence follows the already tracked `Issues.Issue201` route.

Static checks after the correction: `OpenEnroth_GameTest` built; `check_style_OpenEnroth_GameTest` passed; `git diff --check` passed before commits. No headless or graphical run started after the authorization window closed.

Next gate, when the owner renews runtime authorization: run the focused `MM7ScavengerPath.JudgeAndShip` headlessly with the private MM7 data path supplied locally, then the `Issues.Issue201:Issues.Issue2908` controls if the focused route passes:

```sh
build/test/Bin/GameTest/OpenEnroth_GameTest --game mm7 --headless --data-path "$MM7_DATA_PATH" --test-path test/Data --gtest_filter=MM7ScavengerPath.JudgeAndShip --gtest_brief=1 --log-level=error
```

Expected positive branch: six given bits, win bit after the follow-up Judge action, ship offer after Markham dialogue, and Harmondale arrival. Expected negative branch: hat active, win/ship/escape bits false, still on Emerald Island. Revert the last follow-up action locally to confirm the positive assertion fails, then restore it.

Boundary: this covers delivery and ship dialogue, not the six item acquisition routes. Held-item fixtures isolate the Judge's event; one source-backed chest pickup and any trade routes need their own input checks before O2 can be called a full playthrough. A fresh-process save/load after departure is also a separate W4 checkpoint. No private game files, raw scripts, saves, asset inventories, or local logs were committed.
