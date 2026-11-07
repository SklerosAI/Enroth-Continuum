# Codex MM7 artifact handoff — 2026-11-07

- Owner: Codex root. Recipients: Claude MM6 lead and future MM7 maintainer.
- Integrated code: `0e4c03990`, `bbd7cc2f8`, `a6ee11ad5`, `fe5ce5499` on `mm6-mac-implementation`.
- Scope: MM7-only level-seven artifact flag/cap selection in `src/Engine/Objects/Item.cpp`; retain generated item payload in the MM7 branch of `src/Engine/Graphics/Indoor.cpp`; two focused tests in `GameTests_0500.cpp`. MM6's prior shared behavior is title-gated.
- Checks: network-denied `OpenEnroth_GameTest` build passed; MM7 artifact 2/2, MM7 atomic-save 7/7, MM7 controls `Prs.Pr2956`, `Issues.Issue840`, `Prs.Pr2354` 3/3 (12/12 together); MM6 startup/chest guard 1/1. Hidden captures for both titles stayed in the ignored local progress folder.
- Remaining: observe a real chest or indoor pickup and visible staged app behavior. The staged Atlas and installed helper were not replaced for this fix. The isolated atomic save module remains unwired to production SaveLoad/UI.
- Privacy: no proprietary game data, saves, captures or raw logs were committed.
