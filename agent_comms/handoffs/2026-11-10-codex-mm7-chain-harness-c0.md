# MM7 continuous chain harness and C0 handoff

- Branch: `codex/mm7-chain-harness`
- Owner: Codex
- Scope: exact chapter gates in `GameTestMain.cpp`, C0 GameTest, CMake registration and `tools/run_mm7_campaign_chain.py`
- Status: C0 passed headlessly on the local MM7 data. The branch is ready for review and integration.

The marked owner-only system-temp profile is created by the runner and kept after a run. It keeps each numbered save. The runner can resume with `--profile` and removes a marked profile only with `--remove-profile`. The exact C1-C7 gates require the preceding slot, allow earlier numbered slots and reject future slots. Their chapter tests do not exist yet.

C0 starts New Game, briefly skips Margaret's opening tour, then restores that debug setting. It fixtures the six held Scavenger delivery items and equips one strong party setup intended to carry through later chapter saves. It clicks the Judge hand-ins, Markham's ship offer and ship departure. Teleports position the party at Markham's door and the ship. No quest bit, NPC field or map state is written at chapter start. Production Save writes `save000.mm7` after arrival in Harmondale, and native structure preflight accepts it.

Verification in this branch: `OpenEnroth_GameTest` built, C0 passed 1/1 through `tools/run_mm7_campaign_chain.py --through 0`, Python compiled, and `git diff --check` passed. An initial C0 run failed because Margaret's tour intercepted the first door input. A fresh marked profile passed after the tour skip. The failed-run profile can be removed through the runner's explicit remove option. Both local profiles remain outside Git.

Next: review and integrate this branch, then add C1-C7 as the two planned chapter lanes. Run the negative C1 gate with no C0 slot and verify rejection before declaring the harness complete. Update the relevant wiki page and append a separate wiki log commit after integrated verification. The owner asked to wrap work for the week after C0, so no further chapter run was started.
