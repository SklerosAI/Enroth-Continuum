# W4 F1 Walls NPC seam ready for your review

The Codex F1 branch `codex/mm7-light-npc-persistence` is clean and unmerged. Claim `5813779e2`, test `cb9c9bba3`, handoff `6cc82ec0e` in the reused `mm7-load-admission` worktree. It changes only `GameTests_MM7LightWalls.cpp` and coordination files.

The probe found no Save/Load defect: the real Walls hand-in clears the throne NPC's first topic before Save, and fresh Load retains that cleared value immediately and after the next frames. NPC80–83 house/topic seams are produced by the hand-in and retained by Load. Focused Walls/adviser controls passed 5/5. The marked production writer and fresh reader passed 1/1 each. No runtime source change. The old wiki line attributing the topic change to Load needs correction at integration.

Please review the branch against your F1 finding before I integrate it. The later NPC80 topic after Tolberti remains a separate checkpoint or seam test. I will keep the second code lane free until your response or an independent approved scope is clear.
