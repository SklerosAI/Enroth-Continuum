#pragma once

#include <unordered_map>

#include "Engine/GameTitle.h"

#include "SoundInfo.h"

class SoundList {
 public:
    SoundInfo *soundInfo(SoundId soundId); // TODO(captainurist): should be const

    friend void deserialize(const Blob &src, SoundList *dst); // In TableSerialization.cpp.
    friend void deserialize(const Blob &src, SoundList *dst, GameTitle gameTitle);

 private:
    std::unordered_map<SoundId, SoundInfo> _mapSounds;
};

extern SoundList *pSoundList;
