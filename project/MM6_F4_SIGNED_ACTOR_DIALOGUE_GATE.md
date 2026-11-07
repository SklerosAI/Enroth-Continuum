# MM6 signed indoor actor dialogue gate

Status, 2026-11-07: the earlier fixed-follower interpretation is
**superseded** by the [original MM6 map-load correction](MM6_F4_NATIVE_SIGNED_NPC_MAP_LOAD.md).
The signed source marker is cleared on this ineligible actor before gameplay;
its native DLV bytes remain exact. No private map or actor details are recorded here.

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
present in that fresh profile's roster; `getNPCData()` refuses it. That state is **pre-event and pre-map-load NPC generation**. The original MM6
loader clears this marker before gameplay, and the corrected post-world v6
writer now verifies the actor has `npcId == 0`. No real click was run.

A diagnostic assertion that required every interactable signed actor to have
a valid follower address failed **0/1**. The subsequent passing
characterization fixture expects this specific mismatch and the actual lookup
refusal, **1/1**. It is deliberately opt-in and cannot be mistaken for a
playability admission.

## Remaining acceptance

The risk-ranked source no longer reaches the signed follower lookup in the
post-world test state. A real post-world click and any dialogue or hire/fire
transition still need a disposable acceptance test. Eligible native monsters
with nonzero source markers are explicitly refused until the original MM6
StreetNPC generator and positive actor dialogue namespace are implemented.
Keep normal MM6 SaveLoad and general indoor play closed.

The source of truth is the [headless fixture](../test/Bin/GameTest/GameTests_MM6V6IndoorStage.cpp),
[interaction dispatch](../src/Engine/Interaction.cpp),
[game dialogue entry](../src/Application/Game.cpp),
[NPC address resolution](../src/Engine/Objects/NPC.cpp), and
[dialogue hire/fire handling](../src/GUI/UI/UIDialogue.cpp).
Production MM6 SaveLoad, general indoor play and Atlas MM6 Play remain closed.
