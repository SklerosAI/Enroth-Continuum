#pragma once

#include <string_view>

#include "Engine/GameTitle.h"

// One instance belongs to one vendor window, which represents one visit.
// The Atlas supplies the flag only for an MM7 launch with private mods enabled.
class VendorRefreshVisit {
 public:
    bool takeMM7EntryRefresh(GameTitle title, const char *flag) {
        if (_refreshed || title != GameTitle::MM7 || !flag || std::string_view(flag) != "1")
            return false;
        _refreshed = true;
        return true;
    }

 private:
    bool _refreshed = false;
};
