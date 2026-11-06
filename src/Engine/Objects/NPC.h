#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Utility/IndexedArray.h"

#include "Engine/Objects/NPCEnums.h"
#include "Engine/Objects/CharacterEnums.h"

struct NPCData;
struct NPCSacrificeStatus;
struct NPCStats;

enum class NpcType {
    NPC_TYPE_QUEST = 1,
    NPC_TYPE_HIREABLE = 2
};
using enum NpcType;

// TODO(Nik-RE-dev): remove
//----- (0047730C) --------------------------------------------------------
inline bool CheckPortretAgainstSex(int a1, Sex) { return true; }

bool PartyHasDragon();
bool CheckHiredNPCSpeciality(NpcProfession prof);

int UseNPCSkill(NpcProfession profession, int id);

const std::string &GetProfessionActionText(NpcProfession prof);

/**
 * @offset 0x4459F9
 * @param npcId        If positive, means ID of NPC (quest or hireable). If negative, then it's absolute value minus one is index of party hireling.
 * @return             Pointer to NPC data structure
 */
NPCData *getNPCData(int npcId);

/**
 * Restores NPC names from the npcdata.txt name table and hireling names from party data on level load.
 *
 * @offset 0x476C60
 */
void setNPCNamesOnLoad();

/**
 * @offset 0x445C8B
 */
NpcType getNPCType(int npcId);

namespace mm6 {
// The two party slots come first; a table follower at index i is encoded as
// i + 2. This is a roster position, not a stable identity for a hired NPC.
std::vector<std::uint16_t> collectFollowerIndices(const NPCData &first, const NPCData &second,
                                                  const NPCStats &stats);
} // namespace mm6

class FlatHirelings {
 public:
    /**
     * Populates this structure.
     */
    void Prepare();

    /**
     * @return                          Total number of hirelings in this structure.
     */
    size_t Size() const {
        return count;
    }

    /**
     * @param index                     Hireling index.
     * @return                          Whether a hireling is actually a follower (doesn't count towards the total
     *                                  hireling count, e.g. Margaret the guide on Emerald island).
     */
    bool IsFollower(size_t index) const;

    /**
     * @param index                     Hireling index.
     * @return                          Associated `NPCData`.
     */
    NPCData *Get(size_t index) const;

    /**
     * @param index                     Hireling index.
     * @return                          Associated `NPCDarkSacrificeStatus`. Returns null for followers.
     */
    NPCSacrificeStatus *GetSacrificeStatus(size_t index) const;

 private:
    /** Hireling / follower NPC ids.
     * If 0 or 1, then it's an index into `pParty->pHirelings`.
     * If 2 or more, then you have to subtract 2 to get an index into `pNPCStats->pNewNPCData`. */
    std::array<int8_t, 1024> ids = {{}};

    // MM6 has more than 254 active table rows. Keep its wider roster separate
    // so existing MM7/MM8 index and follower behavior is unchanged.
    std::array<std::uint16_t, 503> mm6Ids = {{}};
    bool mm6Roster = false;

    /** Number of valid ids in the array above. */
    size_t count = 0;
};
