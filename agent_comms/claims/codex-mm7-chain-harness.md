# MM7 campaign chain harness

- Owner: Codex root
- Branch/worktree: `codex/mm7-chain-harness` in reused `mm7-load-admission`
- Files owned: `test/Bin/GameTest/GameTestMain.cpp`, `GameTestOptions.{h,cpp}`, `CMakeLists.txt`, new `GameTests_MM7CampaignChainC0.cpp`, `tools/run_mm7_campaign_chain.py`, and focused harness tests if needed.
- Purpose: implement Claude's W4 continuous-chain chapter admission and C0 New Game-to-ship writer. Prove C0 saves a numbered slot and C1 refuses to start without C0's slot.
- Profile policy: an owner-only marked temporary profile begins C0 empty. A chapter starts only when its predecessor's numbered slot is present and no slot of its own or a later chapter exists. Earlier slots are kept, and the runner removes the profile only on `--remove-profile`.
- Checks: C0 headless exact filter, strict missing-predecessor negative, existing disposable Save/Load controls, changed-file style, runner cleanup.
- Conflict check: Claude's next C0-C3 and C4-C7 lanes wait for this harness. His actor-scroll and style branches are already integrated.
