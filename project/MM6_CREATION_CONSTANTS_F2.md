# MM6 creation constants and progression table leads — F2

Status: **pinned primary-source and retained-guide audit, 2026-11-05**. This packet sharpens the [party-creation audit](MM6_CREATION_F2.md) without reopening the owner's original installation or running the partial MM6 engine. The process addresses below come from GrayFace MMExtension commit `9d73c0ff0482bd231ea522f752f4fe9b26640925`; they are candidate memory locations, not verified offsets or values in the owner's executable. Exact game-derived class, skill, character and guide rows remain in ignored local research.

## Table locations to validate in the one F3 read

The pinned [MMExtension MM6 `GameClasses` and `GameClassKinds` definitions](https://github.com/GrayFace/MMExtension/blob/9d73c0ff0482bd231ea522f752f4fe9b26640925/Scripts/Structs/00%20structs.lua#L1081-L1100) distinguish six creation families from eighteen promoted classes. All listed fields are unsigned bytes in the external structure. The [skill recovery field](https://github.com/GrayFace/MMExtension/blob/9d73c0ff0482bd231ea522f752f4fe9b26640925/Scripts/Structs/00%20structs.lua#L140) is a separate signed 16-bit array, not the class HP/SP factor.

| Candidate process address | External shape | F3 question and bound |
| --- | --- | --- |
| `0x4C2630`, `0x4C2638` | Six `HPBase` and six `SPBase` bytes | Map all six family bases to the retained class order; verify the gap and exact executable/version before using any value. |
| `0x4C2640`, `0x4C2654` | Eighteen `HPFactor` and eighteen `SPFactor` bytes | Confirm all three tiers of six families and promotion increments against the manual/guide. Factor labels do not alone prove the final HP/SP formula or retroactivity. |
| `0x4C2668` | Six by seven `StartingStats` bytes | Validate six class vectors, seven-stat ordering and the guide's per-class total/distribution shape; retain exact vectors privately. |
| `0x4C2694` | Six by thirty-one `StartingSkills` bytes | Validate class/skill ordering and MM6 codes: 0 unavailable, 1 granted at start, 2 selectable at start, 3 learnable later. Each row should be checked against the reference card's two fixed and two chosen skills; a code 3 is not a creation choice. |
| `0x4C2750` | Thirteen signed 16-bit `SkillRecoveryTimes` entries | Check independently for melee/recovery implementation; do not confuse it with class or spell delays. |
| `internal.SPStatKinds` | Eighteen `SPStats` bytes, dynamic locator in the source | Determine the actual executable or initialized-state source for each class's mana attribute. The pinned file does not publish a fixed MM6 address for this field. |

The creation-specific arrays lie in a narrow candidate process-memory region from `0x4C2630` through the end of `StartingSkills` at `0x4C274E`; gaps and neighboring data still need validation. F3 must map virtual addresses through the exact owner's PE sections, check raw bounds and edition/patch differences, and compare values to retained text and source meaning. MMExtension may describe patched runtime state. A readable byte range is not, by itself, an original-game rule.

## Guide bounds and current source conflicts

The owner's retained reference card describes two routes after the story: an editable Create Party and a pregenerated Quick Start. It specifies four characters, a shared 50-point stat pool that must be exhausted, one-point adjustments from each class base minus two through 25, two granted plus two selected class-eligible skills, and a class-change reset of starting stats/skills. The owned Prima guide describes a common 73-point seven-stat base distribution per class, but those prose claims do not give each exact ordered vector, Quick Start's party/face/skill/inventory state, or the F3 executable values. The manual/guide describe promotion-related HP/SP increases; the exact base/factor table and final formula must be reconciled separately.

The current C++ [party-creation path](../src/GUI/UI/UIPartyCreation.cpp) has no MM6 Quick Start branch, uses nine inherited class buttons, and initializes an MM7 default party before the screen. [Character::ChangeClass](../src/Engine/Objects/Character.cpp) and skill selection divide class IDs by four into MM7's nine-family availability table; MM6 families have three tiers. [Character::GetMaxHealth/GetMaxMana](../src/Engine/Objects/Character.cpp) use MM7 static class arrays and mana-stat switch. For example, retained MM6 class ID 3 denotes Cleric, while the inherited enum treats 3 as Black Knight; the current `GetMaxMana` switch sends that numeric ID to the no-magic default. This is a deterministic source/ID collision, not an observed MM6 character-sheet value. The inherited starter-book/bottle collisions and portrait/voice questions remain in [creation F2](MM6_CREATION_F2.md).

Trainer availability is a separate service rule. [Manual/service claims](MM6_MANUAL_SERVICES_CLAIMS_F2.md) place MM6 training houses at IDs 79–88, outside the current MM7 cap array's 89–98 range. The class starting-skill codes do not encode all Expert/Master teacher locations, pricing or requirements. F3 must join the original teacher and house data to the class/skill policy; F4 and F5 must test both eligibility and actual training.

## F3, F4 and F5 handoff

- **F3 single read-only preflight:** version/hash the owner's executable and compare the candidate six-family/eighteen-class tables with the retained class/skill order, manual and guide. Extract the exact six stats vectors, six by thirty-one skill codes, family bases and promoted factors privately if the PE mapping validates. Locate `SPStatKinds`, Quick Start fixture, starting inventory/spell flags, face/voice mapping and teacher caps or record each unresolved source with a bounded F4/F5 route. Validate code counts and discrepancies rather than silently substituting MM7 arrays.
- **F4 offline:** implement six-class Create Party and distinct Quick Start state, title-specific HP/SP/stat/skill/mana-source tables, class-change reset, promotion and trainer eligibility. Test all six creation families, 18 promoted class IDs, 31 skill IDs, stat-pool/min/max boundaries, two-granted/two-selected logic, the MM6 Cleric mana collision, starter items/spells, and save/restore. Preserve MM7 tables and profile.
- **F5 graphical:** inspect all six class buttons, stat arrows, optional skill lists, face/voice choices, Quick Start versus custom composition, first map inventory/HP/SP, Expert/Master training and both promotion tiers. Save, quit, relaunch and reload the resulting state; a source-table or synthetic parser pass is not visible play.

The table locations narrow an original-file question. They do not close F2, prove the shipped executable layout, or authorize F3 before the integration gate is frozen.
