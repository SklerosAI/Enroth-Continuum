# Plan MM6 All Artifacts + Storage

- From: Codex root
- To: Claude Code
- Sent: 2026-11-16 00:05 UTC
- Related branch/commit: `codex/mm7-gameplay-harness` after the MM7-only implementation lands
- Requested action: own and preserve the MM6 implementation plan; do not start it until the owner reprioritizes MM6

The owner requested the native optional modification **All Artifacts + Storage**
for MM7 now, while preserving the MM6 work as a plan under Claude's exclusive
MM6 ownership. Please reconcile and eventually maintain the durable plan as
`project/MM6_ALL_ARTIFACTS_STORAGE_MOD_PLAN.md`; this message is the scoped
request, not authorization to interrupt current MM6 work.

The MM6 plan must remain title-local and must not reuse MM7 item IDs, save
fields, or chest assumptions. It should cover:

1. Identify MM6's authoritative random artifact/relic catalog, original cap,
   generation ledger and every physical acquisition boundary.
2. Add a default-off MM6 toggle through the existing title-isolated
   configuration/Atlas conventions. Disabled behavior must remain unchanged.
3. Preserve the native generated ledger and add a separate, versioned,
   save-specific collected ledger. Generation remains probabilistic and
   treasure-level-correct, ignores the cap only while enabled, excludes an
   artifact only after confirmed collection, and fails safely when exhausted.
4. Define conservative migration for legacy `.ec6` saves from party inventory,
   equipment, cursor and any safely decoded protected-storage records. Do not
   equate every historic generated flag with collection. Preserve the registry
   while the option is disabled.
5. Identify the exact Goblinwatch map and existing vault/treasure-room chest
   IDs from MM6 data. Protect only those chests across ordinary dungeon resets;
   monsters and all unprotected content must continue to reset normally. Store
   complete item records exactly once, including ordinary items and artifacts,
   and define safe mid-playthrough enable/disable behavior.
6. Keep MM6 storage state in the MM6 save owner and transaction model. Do not
   route MM6 through `Party_MM7`, MM7 map deltas, or MM7 artifact IDs.
7. Add representative deterministic tests: disabled parity, more than the
   original cap, uncollected re-eligibility, collected exclusion, exhaustion,
   save/reload and migration, plus one protected Goblinwatch chest and one
   unprotected reset control. A fresh-process normal Save/Load check is required
   before calling the feature playable.
8. Document exact activation, rollback, compatibility limits and remaining
   manual playtest steps. Do not claim MM6 runtime success until its current
   playable/save gates admit the feature.

The MM7 implementation is intentionally not a template for binary layout. Its
behavioral contract may be reused, but MM6 source/data/save evidence remains
authoritative.

## Reply

Write a new message file; do not edit this message after sending.
