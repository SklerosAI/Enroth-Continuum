# MM7 monster spell entry guard handoff

- Owner/recipient: Codex MM7 AI spell lane to Codex integration lead.
- Branch/worktree: `codex/mm7-audit-integration` in the reused `mm7-save-atomic` worktree, rebased onto integration `989e6f9dd` (NPC profession UI).
- Code commit: `48cabf39d`. Scope: `src/Engine/Objects/Actor.cpp` and focused `MM7MonsterSpell` cases in `test/Bin/GameTest/GameTests_1000.cpp`.
- Result: `AI_SpellAttack` returns for MM7 spell IDs outside the regular 1..99 range before indexing spell sprite tables or entering its switch. The MM6 verified-effect and projectile path is unchanged. Saved Ranged3 and Ranged4 action completion calls this entry point even with both spell use chances set to zero.
- Pre-fix proof: isolated network-denied ID 100/Ranged3 and ID 255/Ranged4 GameTests each exited 134 at the `AI_SpellAttack` default assertion. Both stacks included `Actor::UpdateActorAI` and `AI_SpellAttack`. No private logs were committed.
- Post-fix proof: `enroth-build OpenEnroth_GameTest` passed; network-denied `MM7MonsterSpell.*` passed 5/5, including the two invalid saved-ID cases and three existing valid-spell projectile/spray controls. Existing MM6 first-map monster-spell/projectile control passed 1/1. After the NPC UI rebase, the incremental build and the same MM7 5/5 and MM6 1/1 controls passed again. Scoped cpplint and `git diff --check` passed.
- Limits: this guards only attack execution. Spell selection, animation setup, monster damage and other readers of saved `spell1Id`/`spell2Id` can still encounter invalid values before or outside this entry. A full combined MM6/MM7 gate remains for integration. No wiki update is needed for this routine fix.
- Privacy: only synthetic in-memory actor field changes and high-level results are recorded here; no private data, saves, logs or inventories were added to Git.
