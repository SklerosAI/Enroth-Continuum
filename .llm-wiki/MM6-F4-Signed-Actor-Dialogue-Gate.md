---
type: analysis
up: "[[MM6-F4-Offline-Seams]]"
related: ["[[MM6-F4-Indoor-Source-Sweep]]", "[[MM6-F4-V6-Indoor-Input-Cycle]]", "[[Campaign-Status]]"]
tags: [mm6, f4, indoor, npc, dialogue, headless]
---

# MM6 F4 Signed Actor Dialogue Gate

A source-backed pre-event fixture finds an interactable signed native indoor actor whose negative follower address is unresolved in a fresh profile.

The [project packet](../project/MM6_F4_SIGNED_ACTOR_DIALOGUE_GATE.md) traces the current path: `InteractWithActor()` queues dialogue for a nonzero actor `npcId`; `Game_StartDialogue()` passes it to `initializeNPCDialogue()`, which calls MM6 `getNPCData()`. A negative ID addresses the current flattened follower roster. The risk-ranked source actor retains exact native identity and passes `CanInteractWithActor()`, but its follower address is out of range and the actual lookup throws. This is a pre-event, read-only result, without a real click, follower mutation or original-game behavior proof.

The deliberate safe-address diagnostic failed **0/1**. The opt-in characterization fixture then passed **1/1**, checking the interaction predicate, roster resolution and lookup refusal. The first, second and risk-ranked test-only v6 five-child chains still pass **15/15** and an MM7 control passes **1/1**. The save mapping remains intact; production dialogue and SaveLoad are unchanged. A post-world clickable dialogue/hire-fire acceptance test and original MM6 interaction rule are required before indoor gameplay is admitted.

See also: [Indoor Source Sweep](MM6-F4-Indoor-Source-Sweep.md), [V6 Indoor Input Cycle](MM6-F4-V6-Indoor-Input-Cycle.md), [Agent Handoff](Agent-Handoff.md), and [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
