#include "MM6Executable.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

#include "Engine/Resources/EngineFileSystem.h"

#include "Utility/Exception.h"
#include "Utility/Sha256.h"
#include "Utility/Unaligned.h"

namespace mm6 {

static constexpr std::string_view GOG_1_2_SHA256 = "b804fda03f56c38cfeecd1a620ac97c2eeebe916e3048189520b34d43688e3e6";

template<class T>
static T readImage(const Blob &image, std::size_t offset) {
    if (offset > image.size() || image.size() - offset < sizeof(T))
        throw Exception("MM6.exe has a truncated PE header");
    return readUnaligned<T>(static_cast<const std::byte *>(image.data()) + offset);
}

Executable::Executable(Blob image) : _image(std::move(image)) {
    const std::string hash = sha256(_image.str());
    if (hash != GOG_1_2_SHA256)
        throw Exception("'{}' is not the MM6.exe of the GOG release of Might and Magic VI 1.2, which MM6 reads its rule tables from. "
                        "Expected SHA-256 {}, got {}.", _image.displayPath(), GOG_1_2_SHA256, hash);

    if (readImage<std::uint16_t>(_image, 0) != 0x5A4D) // "MZ".
        throw Exception("MM6.exe has no DOS header");
    const std::uint32_t pe = readImage<std::uint32_t>(_image, 0x3C);
    if (readImage<std::uint32_t>(_image, pe) != 0x4550) // "PE\0\0".
        throw Exception("MM6.exe has no PE header");
    const int sectionCount = readImage<std::uint16_t>(_image, pe + 6);
    const std::size_t optionalHeader = pe + 24;
    if (readImage<std::uint16_t>(_image, optionalHeader) != 0x10B) // PE32.
        throw Exception("MM6.exe is not a 32-bit PE image");
    const std::uint32_t imageBase = readImage<std::uint32_t>(_image, optionalHeader + 28);
    const std::size_t sectionTable = optionalHeader + readImage<std::uint16_t>(_image, pe + 20);
    for (int i = 0; i < sectionCount; i++) {
        const std::size_t header = sectionTable + 40 * i;
        const std::uint32_t virtualSize = readImage<std::uint32_t>(_image, header + 8);
        const std::uint32_t rawSize = readImage<std::uint32_t>(_image, header + 16);
        Section section;
        section.address = imageBase + readImage<std::uint32_t>(_image, header + 12);
        section.size = virtualSize ? std::min(virtualSize, rawSize) : rawSize; // The rest of a section is zero-filled at load.
        section.fileOffset = readImage<std::uint32_t>(_image, header + 20);
        if (section.fileOffset > _image.size() || _image.size() - section.fileOffset < section.size)
            throw Exception("MM6.exe section {} lies outside the file", i);
        _sections.push_back(section);
    }
}

std::uint8_t Executable::u8(std::uint32_t address) const {
    return readUnaligned<std::uint8_t>(at(address, 1));
}

std::int16_t Executable::i16(std::uint32_t address) const {
    return readUnaligned<std::int16_t>(at(address, 2));
}

std::int32_t Executable::i32(std::uint32_t address) const {
    return readUnaligned<std::int32_t>(at(address, 4));
}

std::uint32_t Executable::u32(std::uint32_t address) const {
    return readUnaligned<std::uint32_t>(at(address, 4));
}

std::optional<std::string> Executable::string(std::uint32_t address) const {
    const Section *section = sectionAt(address);
    if (!section)
        return std::nullopt;
    const char *begin = static_cast<const char *>(_image.data()) + section->fileOffset + (address - section->address);
    const std::string_view rest(begin, section->size - (address - section->address));
    const std::size_t length = rest.find('\0');
    if (length == std::string_view::npos)
        throw Exception("MM6.exe string at 0x{:X} is not terminated", address);
    return std::string(rest.substr(0, length));
}

const Executable::Section *Executable::sectionAt(std::uint32_t address) const {
    for (const Section &section : _sections)
        if (address >= section.address && address - section.address < section.size)
            return &section;
    return nullptr;
}

const std::byte *Executable::at(std::uint32_t address, std::size_t size) const {
    const Section *section = sectionAt(address);
    if (!section || section->size - (address - section->address) < size)
        throw Exception("MM6.exe has no data at 0x{:X}", address);
    return static_cast<const std::byte *>(_image.data()) + section->fileOffset + (address - section->address);
}

const Executable &executable() {
    static const Executable result = [] {
        if (!dfs->exists("mm6.exe"))
            throw Exception("MM6.exe is missing from the MM6 data folder. MM6 reads its rule tables from the MM6.exe of the GOG release, version 1.2.");
        return Executable(dfs->read("mm6.exe"));
    }();
    return result;
}

} // namespace mm6
