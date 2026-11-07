#pragma once

#include <string_view>

namespace mm6 {

// The shipped MM6 outdoor grid has five columns and three rows. A caller
// still has to prove that the named base and delta belong to the loaded
// 67-map archive; this only rules out indoor and invented outdoor names.
inline bool isMM6OutdoorMapName(std::string_view name) {
    return name.size() == 9 && name.substr(0, 3) == "out" &&
        name[3] >= 'a' && name[3] <= 'e' &&
        name[4] >= '1' && name[4] <= '3' &&
        name.substr(5) == ".odm";
}

} // namespace mm6
