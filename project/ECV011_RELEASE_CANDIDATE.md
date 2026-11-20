# ECv0.1.1 candidate

Released on 2026-11-20. This file preserves the pre-tag plan and candidate
checks; [Release Milestones](../.llm-wiki/Release-Milestones.md) records the
final gates, tag and local installation.

`ECv0.1.1` is a proposed MM7 patch release. No tag has been created or moved.
It includes the existing separate MM6 playtest helper, but does not declare
MM6 released or its campaign accepted. MM6 release work continues on
`claude/mm6`; `ECv0.2.0` remains its planned release line. The active MM6
worktree and its private saves and fixtures are outside this candidate.

## Source scope

- MM7 retains its published `ECv0.1.0` save and profile path. The opt-in chest
  mod remains separate from the new click-loot mod.
- The MM7 click-loot mod previews and rerolls random corpse rewards and random
  script items awarded by directly clicked trash heaps, decorations or faces.
  The same in-game checkbox controls both. Fixed rewards and gold use the
  inherited flow. A new game with the mod on prepares the Emerald Isle dragon
  for a rapid owner test.
- Atlas and the MM7 helper declare version `0.1.1`. The build must still be
  produced from the exact release tag after tagging, with no game data in it.

## Release gates

1. Land the MM7 source and wiki commits on `codex/mm7`, then integrate the
   verified release commit to neutral `master` without moving `ECv0.1.0`.
2. Full build, `check_style`, unit tests, full default MM7 headless suite,
   opt-in chest and click-loot tests, MM7 durable save pairs and campaign
   checkpoints. Run MM6 focused controls on this shared-core candidate.
3. Run graphical MM7 acceptance for the popup, checkbox, trash heap and
   dragon, plus a real Save/Load. Confirm the installed MM6 playtest helper
   still starts with its separate profile; the newer MM6 branch is not part
   of this patch candidate unless independently integrated and validated.
4. Once gates pass, create and push annotated `ECv0.1.1` on the chosen master
   commit. Build the exact tag, assemble and sign Atlas and both helpers,
   inspect bundle metadata and contents, install, compare the installed bytes,
   and launch each title. Preserve all local profiles and saves.

The detailed MM6 working gate and current campaign status belong to Claude's
MM6 lane. This candidate does not substitute its headless results for the
remaining owner playthrough.

## Candidate checks on 2026-11-20

- Full default MM7 headless GameTest: 737/737 passed.
- Opt-in MM7 chest, click-loot and artifact combination: 5/5 passed.
- MM6 shared-code controls for item events and decorations: 3/3 passed.
- Unit tests: 1125 passed, 45 skipped because optional private evidence was
  not supplied. `check_style`, Atlas page tests and Swift build-contract tests
  passed.
- The mandatory fresh-process MM7 checkpoint sweep, exact-tag release build,
  installed-app check and owner graphical acceptance remain release gates.
