#pragma once

#include <string>

#include "GUI/GUIWindow.h"

class GUIWindow_QuickReference : public GUIWindow {
 public:
    GUIWindow_QuickReference();
    virtual ~GUIWindow_QuickReference() {}

    virtual void Update() override;

    /**
     * @return                          Reputation line at the bottom of the screen, with its colour tags.
     */
    static std::string reputationLine();
};
