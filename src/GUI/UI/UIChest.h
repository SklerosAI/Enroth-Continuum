#pragma once

#include "GUI/GUIWindow.h"

class GUIWindow_Chest : public GUIWindow {
 public:
    explicit GUIWindow_Chest(int chestId);
    virtual ~GUIWindow_Chest() {}

    virtual void Update() override;

    int chestId() const {
        return _chestId;
    }

 private:
    /**
     * Draws the exit button over the chest or the inventory. MM6 draws the 3D view's side borders under it.
     */
    void drawExitButton();

    int _chestId = 0;
};
