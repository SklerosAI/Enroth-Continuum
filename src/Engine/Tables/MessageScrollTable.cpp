#include "MessageScrollTable.h"

#include <array>
#include <string>
#include <string_view>
#include <utility>

#include "Library/Serialization/Serialization.h"

#include "Utility/Memory/Blob.h"
#include "Utility/String/Split.h"
#include "Utility/String/Transformations.h"

IndexedArray<std::string, ITEM_MM6_FIRST_MESSAGE_SCROLL, ITEM_LAST_MESSAGE_SCROLL> pMessageScrolls;

void initializeMm6MessageScrolls(const Blob &scrolls) {
    int currentId = -1;
    std::string record;
    auto finishRecord = [&] {
        if (currentId < std::to_underlying(ITEM_MM6_FIRST_MESSAGE_SCROLL) || currentId > std::to_underlying(ITEM_LAST_MESSAGE_SCROLL))
            return;
        std::string_view payload(record);
        payload.remove_prefix(payload.find('\t') + 1);
        if (payload.starts_with('"')) {
            size_t end = 1;
            while (end < payload.size()) {
                if (payload[end] == '"' && (end + 1 == payload.size() || payload[end + 1] != '"'))
                    break;
                end += payload[end] == '"' ? 2 : 1;
            }
            pMessageScrolls[static_cast<ItemId>(currentId)] = std::string(payload.substr(1, end - 1));
        } else {
            pMessageScrolls[static_cast<ItemId>(currentId)] = std::string(payload.substr(0, payload.find('\t')));
        }
    };

    for (std::string_view line : split(scrolls.str()).by("\r\n")) {
        size_t tab = line.find('\t');
        int id = 0;
        if (tab != std::string_view::npos && tryDeserialize(line.substr(0, tab), &id)) {
            finishRecord();
            currentId = id;
            record = std::string(line);
        } else if (currentId >= 0) {
            record += "\r\n";
            record += line;
        }
    }
    finishRecord();
}

void initializeMessageScrolls(const Blob &scrolls) {
    // scroll.txt table structure: item index | message text (localized) | scroll title (localized, not used) | (empty).
    for (std::string_view line : split(scrolls.str()).by("\r\n").drop(1).skip("")) {
        std::array<std::string_view, 2> tokens = split(line).by('\t');
        if (tokens[0].empty())
            continue; // Skip tab-only trailing lines.

        ItemId i = static_cast<ItemId>(fromString<int>(tokens[0]));
        pMessageScrolls[i] = unquote(tokens[1]);
    }
}
