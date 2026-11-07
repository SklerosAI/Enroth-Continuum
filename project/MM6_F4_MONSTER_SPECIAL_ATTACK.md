# MM6 monster special attack labels and effect boundary

Status: F4 typed source and pre-effect guard, 2026-11-06. No MM6 special attack has been observed in play.

The accepted F3-matched retained `monsters.txt` has 93 active special-attack rows. The inherited name parser silently mapped 11 of these to `SPECIAL_ATTACK_NONE`: eight `Affraid` rows and three abbreviated poison rows. The MM6 loader now uses an exact title-specific name map after extracting the existing `x<level>` suffix. All 93 active rows decode to a nonempty type in an offline native test. MM7 keeps its inherited parser.

Decoding a label does not prove its effect. `Character::ReceiveSpecialAttackEffect` uses MM7 condition resistance, save checks, inventory break/steal selection, sound, animation and notification rules. For MM6, every nonempty special attack now rejects before observer notification, random selection or character/item mutation. The no-attack value remains allowed; MM7 keeps its effect path. This is a fail-closed boundary that deliberately leaves MM6 combat incomplete.

Next F4 work must establish the original MM6 success and resistance rules, effect level interpretation, condition severity and duration, inventory selection, cues, and save persistence for every active type. Each enabled effect needs a retained-row scenario and a state test proving failure and success behavior. F5 must observe representative conditions, breakage/theft and save/quit/relaunch. The [F2 combat audit](MM6_COMBAT_SEMANTICS_F2.md) and private per-ID register provide the source checklist.

## Break item, 2026-11-07

`BrkItem`, the Goblin King's special attack on the first map, is open. The original executable picks a random unbroken item with an ID from 1 to 134 among all item records, saves against three times the item's Mod2 plus material with the luck bonus, and on a failed save sets the broken bit of the item condition word. The shared candidate and save code match, and the effect runs in an MM6 helper because MM6 has no hardened items and the shared portrait cue indexes MM7 icon frames. The MM6 portrait animation for this effect is not mapped yet. All other special attacks remain closed and now refuse before the hit changes anything.
