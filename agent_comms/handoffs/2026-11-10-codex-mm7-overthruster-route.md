# MM7 Light Overthruster L10–L14 handoff

- Owner: Codex subagent, branch `codex/mm7-overthruster-route` from master `5b84706a0`.
- Claim commit: `c799fcabd`. Code commit: `ff5394f06`.
- Integration: pending parent and Claude review. No merge, push, graphical run, normal profile, wiki, PLAN or runtime edit.

## Covered interactions

New `GameTests_MM7OverthrusterRoute.cpp` and its CMake registration provide four focused cases. The L9 request is selected through the Lambent throne dialogue, then the two quest chests are opened and looted through game input; the Oracle wetsuit bit and at least four acquired suits are checked. At Avlee's west edge, walking with one unsuited party member stays in Avlee, while four suited members can accept travel to Shoals. The Shoals entrance interaction opens the Lincoln transition. In Lincoln, the unpowered pedestal does not grant the item; clicking the power console enables the pedestal, which grants the held Overthruster and both retrieval bits. The Lincoln exit refuses the party with one missing suit, then accepts after the fourth suit is equipped and returns to Shoals. Quest and map tapes check the transitions.

The L8-to-L9 link is a fixture: `QBIT_XENOFEX_QUEST_FINISHED` and NPC80's second dialogue topic are set before the throne interaction. The Lincoln case also fixtures the accepted L9 request and three equipped wetsuits; the four cases are chapter slices, not uninterrupted traversal. Its item assertion includes the party's held item, because the pedestal grants the Overthruster there. Input-driven chest collection and the border/exit suit gates are independently verified, but equipping the collected chest suits across all four characters is not linked into one run.

## Focused evidence

- `OpenEnroth_GameTest` built and linked.
- `MM7OverthrusterRoute.*` passed 4/4.
- Combined `MM7OverthrusterRoute.*:Issues.Issue402:Issues.Issue403_970` passed 6/6.
- Changed-file `cpplint.py` and `git diff --check` passed.
- Private MM7 data and raw logs remained local outside Git.

## Open boundary

L15's throne hand-in triggers the ending and is a separate scope; `MM7Campaign.Endings` already tests it with a fixture item. A later test must link the retrieved item to that hand-in and run the fresh-process Save/Load checkpoint after retrieval. Natural L8 hand-in to L9 topic continuity also remains for a follow-up link. No ending or checkpoint is claimed here.
