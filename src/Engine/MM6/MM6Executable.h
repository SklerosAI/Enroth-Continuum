#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "Utility/Memory/Blob.h"

namespace mm6 {

/**
 * `MM6.exe` of the GOG release of Might and Magic VI, version 1.2. Its data sections hold the tables the MM6 runtime
 * plays by: class creation arrays, spell costs and recovery, house rooms, transport routes and shop stock. Addresses
 * are virtual addresses, as the original code uses them.
 */
class Executable {
 public:
    /**
     * @param image                     Contents of `MM6.exe`.
     * @throws Exception                When `image` is not the GOG 1.2 `MM6.exe`.
     */
    explicit Executable(Blob image);

    [[nodiscard]] std::uint8_t u8(std::uint32_t address) const;
    [[nodiscard]] std::int16_t i16(std::uint32_t address) const;
    [[nodiscard]] std::int32_t i32(std::uint32_t address) const;
    [[nodiscard]] std::uint32_t u32(std::uint32_t address) const;

    /**
     * @param address                   Address of a zero-terminated string.
     * @return                          The string, or `std::nullopt` when the address is not backed by the file, as
     *                                  for buffers the game fills at runtime.
     */
    [[nodiscard]] std::optional<std::string> string(std::uint32_t address) const;

 private:
    struct Section {
        std::uint32_t address = 0;
        std::uint32_t size = 0;
        std::uint32_t fileOffset = 0;
    };

    [[nodiscard]] const Section *sectionAt(std::uint32_t address) const;
    [[nodiscard]] const std::byte *at(std::uint32_t address, std::size_t size) const;

 private:
    Blob _image;
    std::vector<Section> _sections;
};

/**
 * @return                              `MM6.exe` from the game data folder, read and checked on first use.
 */
const Executable &executable();

} // namespace mm6
