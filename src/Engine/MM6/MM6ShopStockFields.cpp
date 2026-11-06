#include "MM6ShopStockFields.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace mm6 {
namespace {

constexpr std::array<char, 4> SHOP_TAG = {'S', 'H', 'O', 'P'};
constexpr std::uint8_t SHOP_VERSION = 1;
constexpr std::size_t SHOP_PAYLOAD_SIZE = 1 + SHOP_STOCK_RECORD_COUNT * 28;

} // namespace

ShopStockFields captureShopStock(const ShopStockFields &owner, GameTitle title) {
    if (title != GameTitle::MM6 || !owner.ready)
        throw std::runtime_error("MM6 shop stock requires a ready title-local owner");
    return owner;
}

SaveSection encodeShopStockSection(const ShopStockFields &fields) {
    if (!fields.ready)
        throw std::runtime_error("MM6 SHOP requires a ready title-local owner");
    SaveSection section{SHOP_TAG, true, {}};
    section.payload.reserve(SHOP_PAYLOAD_SIZE);
    section.payload.push_back(SHOP_VERSION);
    for (const ShopStockRecord &record : fields.records)
        section.payload.insert(section.payload.end(), record.begin(), record.end());
    return section;
}

ShopStockFields decodeShopStockSection(const SaveContainer &container) {
    const auto it = std::find_if(container.sections.begin(), container.sections.end(),
                                 [](const SaveSection &section) { return section.tag == SHOP_TAG; });
    if (it == container.sections.end() || !it->required ||
        it->payload.size() != SHOP_PAYLOAD_SIZE || it->payload[0] != SHOP_VERSION)
        throw std::runtime_error("MM6 save lacks a valid required SHOP section");

    ShopStockFields staged;
    staged.ready = true;
    std::size_t offset = 1;
    for (ShopStockRecord &record : staged.records) {
        std::copy_n(it->payload.begin() + offset, record.size(), record.begin());
        offset += record.size();
    }
    return staged;
}

void loadShopStockBeforeMutation(std::span<const std::uint8_t> bytes,
                                 ShopStockFields *destination, GameTitle title) {
    if (title != GameTitle::MM6 || !destination)
        throw std::runtime_error("MM6 SHOP load requires a title-local destination");
    ShopStockFields staged = decodeShopStockSection(decodeSaveContainer(bytes));
    *destination = std::move(staged);
}

} // namespace mm6
