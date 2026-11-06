# MM6 F4 Quick Start static audit

Status: **identity-matched static executable evidence, 2026-11-06; no runtime Quick Start implementation or MM6 launch**. The detailed offsets, values and disassembly are in an ignored owner-local `F4_QUICK_START_PE_AUDIT_PRIVATE.md` beside the accepted F3 executable reports. This packet contains no game payload, localization text or exact character fixture.

## Route and fixture evidence

The accepted F3 hash of the staged MM6 executable matched before this read-only audit. Its opening choice constructs separate Create Party and Quick Start buttons. The input handler assigns distinct states; the caller sends Create Party through the editable creation routine and Quick Start through a separate finalizer. Both branches share a preceding party reset and subsequent game initialization. The current engine's `GameStarterOptions.quickStart` is an application startup shortcut and does **not** represent this game choice.

The reset and finalizer identify four ordered characters. For each, the audit recovered a starting class, face, localized name reference, seven stat writes and four learned skill writes. The four stat vectors all respect the corrected F3 class bases and the documented per-stat bounds; their combined allocation spends exactly the shared 50 points. Each skill set consists of two class-granted and two class-selectable skills according to the corrected F3 six-family matrix. These are static source facts, not observed game-screen results.

The finalizer then walks learned skills to create equipment, inventory and first-spell state. It calls item placement helpers and a random-number routine. A first-spell flag and starter book are visible for the magic-school branches, but the complete item identities, slot placement, random outcomes and final spell state have not been reconciled with retained item/spell tables and subsequent initialization. The reset also chooses other per-character fields through source paths whose voice meaning is not established. No exact voice value is claimed.

## Unlock checklist

1. Trace each of the sixteen learned-skill cases through item construction, placement, and retained MM6 item identities; bound all random variants and failure paths.
2. Reconcile the first-spell byte and book paths with the retained 99-spell map, then inspect any later initialization that changes spell state.
3. Resolve face-to-voice state and remaining per-character defaults from the accepted executable and character wire layout.
4. Implement a distinct, MM6-only Quick Start action from the original opening choice with the fully verified fixture; preserve editable Create Party and MM7 behavior.
5. Verify initial state, inventory, character sheets, and `.ec6` save/reload offline before permitting an MM6 game launch; inspect the graphical choice and first party in F5.

The existing MM6 default party remains provisional. This audit does not authorize replacing it with partial Quick Start data or enabling the button. The [creation foundation](MM6_F4_CREATION_RULES_FOUNDATION.md), [creation F2 audit](MM6_CREATION_F2.md), [F3 executable follow-up](MM6_F3_EXECUTABLE_FOLLOWUP.md), and [F4 build graph](MM6_F4_BUILD_GRAPH.md) provide adjacent contracts.

## Starter item and voice follow-up

A second read-only pass checked all **16 learned-skill passes** against the identity-matched executable and retained item table. Seven passes write starter equipment records and equip links; six set a first-spell flag and place a corresponding book; three place a bottle and one of three random herb variants. The first character also receives a separate opening letter. Independently, the finalizer calls a weighted random-item generator **once per character before the skill loop**. Its candidate scan and modifier paths prevent a single fixed-inventory claim. An ignored owner-local checker verified the sixteen PE dispatches, literal roles, retained item categories and random call sites; its aggregate check passed. Exact IDs, addresses, names, disassembly and checker remain local.

The [pinned MMExtension player schema](https://github.com/GrayFace/MMExtension/blob/9d73c0ff0482bd231ea522f752f4fe9b26640925/Scripts/Structs/00%20structs.lua#L1390-L1500) independently maps MM6's face, skill, spell, item, inventory and equipment fields to the observed executable writes. It defines a separate Voice field for MM7/8, with no corresponding MM6 field. **Inference:** MM6 voice identity may be derived from the face or another runtime path; the original sound-selection read still needs a direct trace. The executable's later initialization creates a new map archive and changes party header state, then enters the game loop; a downstream callback and map-entry effects have not been fully followed for character or inventory changes.

The runtime Quick Start route and private-manifest candidate remain disabled. To unlock them, enumerate the random item's eligible pool and modifier transitions, prove inventory geometry and equip-slot assignment, trace face-to-sound selection, and reconcile downstream initialization plus `.ec6` persistence. The static checker is evidence for these bounded source paths, not a rendered or saved party.
