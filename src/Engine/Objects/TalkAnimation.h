#pragma once

#include "Core/Time/Duration.h"

#include "CharacterEnums.h"

class TalkAnimation {
 public:
    void init();
    void update(Duration dt);

    int currentFrameIndex() const;
    PortraitId currentPortrait() const { return _currentPortrait; }

 private:
    PortraitId _currentPortrait = PORTRAIT_INVALID;
    Duration _currentTime;
};
