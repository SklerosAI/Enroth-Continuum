# MM6 F4 monster preference seam

Status: **title-specific offline rule, 2026-11-06**. This change does not establish the original target-selection probabilities, multiple-victim behavior, or a live encounter result.

The retained F3-matched `monsters.txt` has 173 rows, including six attack-preference cells containing `M` and six containing `F`. The [F2 effect audit](MM6_MONSTER_EFFECT_SEAMS_F2.md) records the direct guide-page check that `M` denotes male targets in MM6. The inherited `MonsterStats::Initialize` map gave `m` the MM7 Monk meaning. The parser now selects male for MM6 `m` and retains Monk for MM7/MM8. It already maps MM6 `f` to female.

`Character::matchesAttackPreference` now compares the six MM6 class flags against MM6's three-ID families. With the promotions option enabled, both promoted tiers match their starting family; without it, only the starting class matches. Ranger, Thief, and Monk flags cannot match an MM6 class. Gender matching continues through the shared character branch. MM7/MM8 keep their existing four-ID class matching.

Focused unit checks cover all six families, both promotion tiers, MM7-only class flags, an out-of-range class ID, and the title-specific `M` token interpretation. The retained-row aggregate check is research evidence, not a runtime encounter. F4 still needs original probability, multi-target and AI rules; F5 needs a mixed-sex party and promotion-targeting encounter with graphical observation.
