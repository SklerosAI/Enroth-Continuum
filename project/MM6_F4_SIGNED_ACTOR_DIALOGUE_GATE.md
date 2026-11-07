# MM6 signed indoor actor dialogue gate

Status, 2026-11-07: the risk-ranked indoor source case has a native actor with
an exact retained negative `npcId`. The test-only v6 GNPC/IART save and replay
chain accepts that fixed actor identity, but this does not establish safe
follower dialogue. No private map name, actor ID, roster size, asset or save
bytes are recorded here.

## Source and runtime path

`InteractWithActor()` first requires `CanInteractWithActor()` and sends
`UIMSG_StartNPCDialogue` for any nonzero actor `npcId`.
`Game_StartDialogue()` passes that signed ID to `initializeNPCDialogue()`,
which calls `getNPCData()` before creating the dialogue window. For MM6,
`getNPCData()` resolves a negative ID as a one-based index into the current
`FlatHirelings` roster; an out-of-range index throws. The same roster-based
interpretation controls `getNPCType()`, while the hire/fire UI avoids removing
a speaking actor when its `npcId` is negative. The negative ID is therefore
not a stable table NPC identity by itself.

The opt-in read-only pre-event fixture reconstructs the selected source in the
real indoor loader and retains exact native actor identity. It then checks the
actor interaction predicate, current follower roster resolution, and actual
`getNPCData()` behavior without starting dialogue or mutating the roster.
The selected signed actor passes `CanInteractWithActor()`, but its index is not
present in that fresh profile's roster; `getNPCData()` refuses it. A direct
interaction in that state would enter the dialogue lookup path and fail.
This is a **pre-event** finding; map events and a real click were not run.

A diagnostic assertion that required every interactable signed actor to have
a valid follower address failed **0/1**. The subsequent passing
characterization fixture expects this specific mismatch and the actual lookup
refusal, **1/1**. It is deliberately opt-in and cannot be mistaken for a
playability admission.

## Acceptance needed before dialogue play

1. Establish the original MM6 meaning of a signed native actor ID in this
   interaction context, including whether the actor should enter dialogue or
   use another source-backed interaction rule. Do not coerce it to zero or
   silently drop the click.
2. At a post-world, clickable checkpoint, prove the actor-to-target lookup
   and dialogue/hire-fire behavior with a disposable profile and without
   changing native identity or corrupting the follower roster.
3. Turn the read-only mismatch characterization into a safe-interaction
   assertion, then rerun the first, second and risk-ranked five-child indoor
   v6 chains and an MM7 control. Preserve the existing strict save gates.

The source of truth is the [headless fixture](../test/Bin/GameTest/GameTests_MM6V6IndoorStage.cpp),
[interaction dispatch](../src/Engine/Interaction.cpp),
[game dialogue entry](../src/Application/Game.cpp),
[NPC address resolution](../src/Engine/Objects/NPC.cpp), and
[dialogue hire/fire handling](../src/GUI/UI/UIDialogue.cpp).
Production MM6 SaveLoad, general indoor play and Atlas MM6 Play remain closed.
