---
type: synthesis
up: "[[Agent-Handoff]]"
related: ["[[MM6-F4-Border-Autosave-Transaction]]", "[[MM7-Atomic-Save-File]]"]
tags: [mm6, transition, death, candidate]
---

# MM6 Transition and Death Candidates

Two Codex-owned MM6 candidate scopes add a Cancel-only indoor transition dialog and a detached party-death model; neither opens production travel or death dispatch.

The [transition dialog](../src/GUI/UI/MM6TransitionDialog.cpp) selects MM6-local exit art, background, button art and `trans.txt` text. Original-executable call-site checking established one-based picture IDs 1–8 for the eight proved icon names; ID 0 is a no-icon sentinel. The first-map event's ID 5 selects the fifth icon, and active ID 8 selects the eighth. Other IDs and missing local house data fall back to Cancel. Every MM6 dialog offers **Cancel only** because the live Accept handler calls `onMapLeave()` before the guarded transition save; the MM6 lead reserved the preflight and Yes admission for later integration. Two focused units passed, and one isolated headless first-map event test opened the dialog, checked art and Cancel, then checked unchanged map and Party after cleanup. Those checks passed again after correcting the one-based indexing. The fixture starts at the native event opcode; walking, Space input, the actual Cancel message dispatch and an offscreen dialog capture remain unproved. MM7 still uses its existing transition path.

The [party-death model](../src/Engine/MM6/MM6PartyDeath.cpp) stages a detached Party and respawn destination. Its accepted-original-MM6 evidence supports a seven-day advance, one death count, New Sorpigal arrival, ordinary gold/HP loss and Guardian Angel Expert/Master outcomes. Four focused units passed. The runtime Guardian Angel slot and MM6 buff-ID join remain unproved, so the model explicitly refuses unsupported states and has no `Game.cpp` hook.

After these edits, the seven-child first-map v5 chain, both full-retry fault children and MM7 control passed in fresh processes. This is a regression gate, not a claim that MM6 first-map play, travel or death is production-ready. The branch commits and exact integration limits are recorded in `agent_comms/handoffs/`.

See also: [Agent Handoff](Agent-Handoff.md), [MM6 Border Autosave Transaction](MM6-F4-Border-Autosave-Transaction.md), [MM7 Atomic Save File](MM7-Atomic-Save-File.md).
