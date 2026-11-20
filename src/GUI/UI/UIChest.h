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

    /**
     * @return                          Shown page of an MM6 chest, 0 for the first.
     */
    int page() const {
        return _page;
    }

    /**
     * Shows the next page of an MM6 chest, or the first after the last.
     */
    void showNextPage();

 private:
    /**
     * Draws the exit button over the chest or the inventory. MM6 draws the 3D view's side borders under it.
     */
    void drawExitButton();

    /**
     * @return                          Pages of the MM6 chest.
     */
    int pageCount() const;

    /**
     * Shows the page button of an MM6 chest on the chest screen while the chest has more than one page, and keeps the
     * shown page among the chest's pages.
     */
    void updatePageButton();

    int _chestId = 0;
    int _page = 0;
    GUIButton *_pageButton = nullptr;
};
