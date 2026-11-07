---
type: synthesis
up: "[[Campaign-Status]]"
related: ["[[MM7-Atomic-Save-File]]", "[[Agent-Handoff]]"]
tags: [mm7, artifacts, treasure, gameplay]
---

# MM7 Level-Seven Artifacts

The integration tree now tracks and caps MM7 artifacts generated from level-seven treasure, and keeps the item inside an indoor world sprite so it can be picked up.

The inherited [picker](../src/Engine/Objects/Item.cpp) filtered previously generated artifacts but did not mark a newly selected one or apply `ArtifactLimit`. The [chest caller](../src/Engine/Objects/Chest.cpp) and indoor random spawn both use it. The indoor caller also cleared the item payload before creating the world sprite. The fix counts MM7 flags, refuses at a nonzero cap, marks a successful choice, and retains the MM7 indoor payload. The existing MM6 selection and indoor behavior stay title-gated.

Two [focused GameTests](../test/Bin/GameTest/GameTests_0500.cpp) passed in a source build on 2026-11-07. One checks two unique, marked artifacts and a cap refusal; the other checks that a newly created indoor sprite contains the generated item. Seven MM7 atomic-save tests and three MM7 controls passed in the same run, 12/12 total. An MM6 startup/chest-guard control passed 1/1. Hidden offscreen captures were saved only in the ignored local progress folder. A real chest or indoor pickup and the staged graphical app still need observation; the installed app and frozen MM7 baseline were not changed.

See also: [Campaign Status](Campaign-Status.md), [MM7 Atomic Save File](MM7-Atomic-Save-File.md), [Agent Handoff](Agent-Handoff.md).
