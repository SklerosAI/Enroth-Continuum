---
type: synthesis
up: "[[index_Enroth-Continuum]]"
tags: [mm6, f4, creation, quick-start]
extends: "[[MM6-F4-Creation-Rules]]"
---

# MM6 F4 Quick Start

An identity-matched static MM6 executable trace separates the original Quick Start choice from editable Create Party and identifies the four-character setup without activating it in Enroth Continuum.

The opening buttons lead to distinct state and caller branches after a shared reset. The Quick Start finalizer writes each character's class, face, localized name, seven stats and four skills. The four stat allocations total the documented 50-point pool, satisfy the corrected class bases and bounds, and follow two granted plus two selectable class skills. The exact fixture and code locators are kept in an ignored owner-local audit.

The finalizer generates starting inventory by learned skill, invokes item placement helpers and uses randomness. A second static pass verified all sixteen learned-skill passes: seven equipment grants, six first-spell/book grants, and three bottle-plus-random-herb grants. A separate weighted random item is generated for each character, and one opening letter is added. An ignored local checker matched the dispatches and item categories without exposing exact IDs. The complete random-item outcomes, equipped slots and later initialization effects are still unproved. [The public audit](../project/MM6_F4_QUICK_START_AUDIT.md) gives the bounded unlock checklist. The current default party remains provisional; `GameStarterOptions.quickStart` is only an application initialization option.

The [pinned MMExtension player schema](https://github.com/GrayFace/MMExtension/blob/9d73c0ff0482bd231ea522f752f4fe9b26640925/Scripts/Structs/00%20structs.lua#L1390-L1500) confirms the observed MM6 face, spell and inventory offsets. It lists a separate Voice field for MM7/8 only. A later direct original-executable trace proved that the MM6 face-animation sound path reads the face byte, combines it with a sound kind and random subvariant, and calls the audio player. The 12×44 count matrix yields 633 possible IDs; 626 distinct IDs are retained in the sound table, so resource resolution remains necessary. The detached [numeric selector](../src/Engine/MM6/MM6VoiceSound.h) has no playback or live Quick Start caller.

The post-finalizer callback writes party header and archive state, then calls a snapshot writer; the actual first outdoor map is loaded in the following game-loop path. The snapshot writer has no inspected direct writes to the four character records. A retained first-map script check verified 411 frames and 11 reload triggers. Under the inherited step-after-trigger rule, ten suffixes can update sprites and one can display a movie; none contains a character-field write opcode. The original executable's reload dispatch and full loader callback closure have not been proved, so **full post-entry character and inventory preservation is still unproved**.

Evidence level: **static source semantics, bounded first-map script check and detached numeric selector**. There was no MM6 process, rendered opening screen, audible cue, live item state, or save/reload validation. Do not activate Quick Start until the original dispatch, complete character/inventory state, input route and durable save transaction are proved.

See also: [MM6 F4 Creation Rules](MM6-F4-Creation-Rules.md), [MM6 F3 Read-Only Preflight](MM6-F3-Read-Only-Preflight.md), [Source and Privacy Boundary](Source-and-Privacy-Boundary.md).
