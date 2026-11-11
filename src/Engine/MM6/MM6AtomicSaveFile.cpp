#include "MM6AtomicSaveFile.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <set>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

#include "MM6KnownSaveFields.h"
#include "MM6RawSaveFields.h"

namespace mm6 {
namespace {

constexpr std::uintmax_t MAX_SAVE_BYTES = 64 * 1024 * 1024;

enum class SlotSchema { KnownV1, RawV2, FullV3, FullV4, FullV5, FullV6Outdoor, NumberedV5OrV7 };

class InvalidSaveFile : public std::runtime_error {
 public:
    using std::runtime_error::runtime_error;
};

void validateSlot(std::string_view slot) {
    if (slot.size() < 5 || slot.size() > 63 || !slot.ends_with(".ec6"))
        throw std::invalid_argument("invalid MM6 save slot name");
    for (char c : slot.substr(0, slot.size() - 4)) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-'))
            throw std::invalid_argument("invalid MM6 save slot name");
    }
}

std::filesystem::path savesDirectory(const std::filesystem::path &profileRoot,
                                     std::string_view slot) {
    validateSlot(slot);
    if (!profileRoot.is_absolute() || profileRoot.empty())
        throw std::invalid_argument("MM6 native profile path must be absolute");
    return profileRoot / "saves";
}

bool regularFileExists(const std::filesystem::path &path) {
    std::filesystem::file_status status = std::filesystem::symlink_status(path);
    if (status.type() == std::filesystem::file_type::not_found)
        return false;
    if (!std::filesystem::is_regular_file(status))
        throw std::runtime_error("MM6 save path is not a regular file");
    return true;
}

// The atomic layer owns only the v3 envelope and its v2-compatible HEAD.
// ACTR/GNPC/RNGS payload semantics belong to the required caller validator.
SaveHeaderFields decodeV3SlotShell(std::span<const std::uint8_t> bytes) {
    SaveContainer container = decodeSaveContainer(bytes);
    constexpr std::array<std::array<char, 4>, 3> extraTags = {{
        {'A', 'C', 'T', 'R'}, {'G', 'N', 'P', 'C'}, {'R', 'N', 'G', 'S'},
    }};
    if (container.version != 3 || container.sections.size() != 22)
        throw std::runtime_error("MM6 v3 slot has wrong version or section count");
    SaveContainer raw;
    raw.version = 2;
    for (const SaveSection &section : container.sections) {
        if (!section.required)
            throw std::runtime_error("MM6 v3 slot has an optional section");
        if (std::find(extraTags.begin(), extraTags.end(), section.tag) == extraTags.end())
            raw.sections.push_back(section);
    }
    if (raw.sections.size() != 19)
        throw std::runtime_error("MM6 v3 slot lacks one of its required owner sections");
    for (const auto &tag : extraTags)
        if (std::none_of(container.sections.begin(), container.sections.end(),
                         [&tag](const SaveSection &section) { return section.tag == tag; }))
            throw std::runtime_error("MM6 v3 slot lacks an active owner section");
    return decodeRawSave(encodeSaveContainer(raw), GameTitle::MM6).metadata.header;
}

SaveHeaderFields decodeV4SlotShell(std::span<const std::uint8_t> bytes) {
    SaveContainer container = decodeSaveContainer(bytes);
    constexpr std::array<char, 4> eventTag = {'E', 'V', 'R', 'T'};
    if (container.version != 4 || container.sections.size() != 23)
        throw std::runtime_error("MM6 v4 slot has wrong version or section count");
    const auto event = std::find_if(container.sections.begin(), container.sections.end(),
                                    [&eventTag](const SaveSection &section) {
                                        return section.tag == eventTag;
                                    });
    if (event == container.sections.end() || !event->required)
        throw std::runtime_error("MM6 v4 slot lacks required EVRT");
    container.sections.erase(event);
    container.version = 3;
    // Reuse the vetted strict v3 shell for the other 22 required tags and
    // v2-compatible HEAD. The semantic callback validates EVRT and MAPS.
    return decodeV3SlotShell(encodeSaveContainer(container));
}

SaveHeaderFields decodeV5SlotShell(std::span<const std::uint8_t> bytes) {
    SaveContainer container = decodeSaveContainer(bytes);
    constexpr std::array<char, 4> spriteTag = {'S', 'P', 'R', 'T'};
    constexpr std::array<char, 4> actorTag = {'A', 'R', 'T', 'R'};
    constexpr std::array<char, 4> decorTag = {'D', 'E', 'C', 'R'};
    if (container.version != 5 || container.sections.size() != 26)
        throw std::runtime_error("MM6 v5 slot has wrong version or section count");
    const auto sprites = std::find_if(container.sections.begin(), container.sections.end(),
                                      [&spriteTag](const SaveSection &section) {
                                          return section.tag == spriteTag;
                                      });
    if (sprites == container.sections.end() || !sprites->required)
        throw std::runtime_error("MM6 v5 slot lacks required SPRT");
    container.sections.erase(sprites);
    const auto actors = std::find_if(container.sections.begin(), container.sections.end(),
                                    [&actorTag](const SaveSection &section) {
                                        return section.tag == actorTag;
                                    });
    if (actors == container.sections.end() || !actors->required)
        throw std::runtime_error("MM6 v5 slot lacks required ARTR");
    container.sections.erase(actors);
    const auto decorations = std::find_if(container.sections.begin(), container.sections.end(),
                                          [&decorTag](const SaveSection &section) {
                                              return section.tag == decorTag;
                                          });
    if (decorations == container.sections.end() || !decorations->required)
        throw std::runtime_error("MM6 v5 slot lacks required DECR");
    container.sections.erase(decorations);
    container.version = 4;
    return decodeV4SlotShell(encodeSaveContainer(container));
}

SaveHeaderFields decodeV6OutdoorSlotShell(std::span<const std::uint8_t> bytes) {
    SaveContainer container = decodeSaveContainer(bytes);
    constexpr std::array<char, 4> visitedTag = {'V', 'I', 'S', 'T'};
    if (container.version != 6 || container.sections.size() != 27 ||
        container.sections.back().tag != visitedTag ||
        !container.sections.back().required)
        throw std::runtime_error("MM6 outdoor v6 slot lacks exact VIST shell");
    container.sections.pop_back();
    container.version = 5;
    // The caller must then decode VIST and validate both maps against source.
    return decodeV5SlotShell(encodeSaveContainer(container));
}

/**
 * @param bytes                         Bytes of a v7 slot file.
 * @return                              HEAD of the nineteen v2 raw owners. Throws unless they are followed by a
 *                                      required PRKM, whose payload is left to the caller's validator.
 */
SaveHeaderFields decodeV7SlotShell(std::span<const std::uint8_t> bytes) {
    SaveContainer container = decodeSaveContainer(bytes);
    constexpr std::array<char, 4> parkedTag = {'P', 'R', 'K', 'M'};
    if (container.version != 7 || container.sections.size() != 20 ||
        container.sections.back().tag != parkedTag || !container.sections.back().required)
        throw std::runtime_error("MM6 v7 slot lacks exact PRKM shell");
    container.sections.pop_back();
    container.version = 2;
    return decodeRawSave(encodeSaveContainer(container), GameTitle::MM6).metadata.header;
}

SaveHeaderFields decodeNumberedSlotShell(std::span<const std::uint8_t> bytes) {
    if (decodeSaveContainer(bytes).version == 7)
        return decodeV7SlotShell(bytes);
    return decodeV5SlotShell(bytes);
}

std::vector<std::uint8_t> readBoundedFile(const std::filesystem::path &path) {
    const std::uintmax_t fileSize = std::filesystem::file_size(path);
    if (fileSize > MAX_SAVE_BYTES)
        throw InvalidSaveFile("MM6 save file exceeds size limit");
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("cannot open MM6 save file");
    const auto size = static_cast<size_t>(fileSize);
    std::vector<std::uint8_t> bytes(size);
    if (size && !input.read(reinterpret_cast<char *>(bytes.data()), size))
        throw std::runtime_error("cannot read complete MM6 save file");
    if (input.peek() != std::char_traits<char>::eof())
        throw std::runtime_error("MM6 save file changed during read");
    return bytes;
}

std::vector<std::uint8_t> readValidatedFile(const std::filesystem::path &path,
                                            SlotSchema schema,
                                            SaveHeaderFields *header = nullptr) {
    std::vector<std::uint8_t> bytes = readBoundedFile(path);
    try {
        if (schema == SlotSchema::KnownV1) {
            KnownSaveFields fields = decodeKnownSave(bytes);
            if (header)
                *header = std::move(fields.metadata.header);
        } else if (schema == SlotSchema::RawV2) {
            RawSaveFields fields = decodeRawSave(bytes, GameTitle::MM6);
            if (header)
                *header = std::move(fields.metadata.header);
        } else if (schema == SlotSchema::FullV3) {
            if (header)
                *header = decodeV3SlotShell(bytes);
            else
                (void) decodeV3SlotShell(bytes);
        } else if (schema == SlotSchema::FullV4) {
            if (header)
                *header = decodeV4SlotShell(bytes);
            else
                (void) decodeV4SlotShell(bytes);
        } else if (schema == SlotSchema::FullV5) {
            if (header)
                *header = decodeV5SlotShell(bytes);
            else
                (void) decodeV5SlotShell(bytes);
        } else if (schema == SlotSchema::NumberedV5OrV7) {
            if (header)
                *header = decodeNumberedSlotShell(bytes);
            else
                (void) decodeNumberedSlotShell(bytes);
        } else {
            if (header)
                *header = decodeV6OutdoorSlotShell(bytes);
            else
                (void) decodeV6OutdoorSlotShell(bytes);
        }
    } catch (const std::runtime_error &) {
        throw InvalidSaveFile("MM6 save schema validation failed");
    }
    return bytes;
}

// A v3 operation must not replace a valid or partially damaged older EC6
// container under the same basename. Inspect both files before any recovery
// or write; a bad checksum still leaves the versioned header recognizable.
void rejectForeignSlotFiles(const std::filesystem::path &profileRoot,
                            std::string_view slotName,
                            std::initializer_list<std::uint16_t> expectedVersions) {
    const std::filesystem::path directory = savesDirectory(profileRoot, slotName);
    const std::filesystem::file_status status = std::filesystem::symlink_status(directory);
    if (status.type() == std::filesystem::file_type::not_found)
        return;
    if (!std::filesystem::is_directory(status))
        throw std::runtime_error("MM6 save path is not a native directory");
    constexpr std::array<std::uint8_t, 8> magic = {'E', 'C', '6', 'S', 'A', 'V', 'E', 0};
    for (const std::filesystem::path &path : {
             directory / std::string(slotName),
             directory / (std::string(slotName) + ".bak")}) {
        if (!regularFileExists(path))
            continue;
        std::ifstream input(path, std::ios::binary);
        if (!input)
            throw std::runtime_error("cannot inspect MM6 save file");
        std::array<std::uint8_t, 10> prefix{};
        if (!input.read(reinterpret_cast<char *>(prefix.data()), prefix.size()))
            continue; // Truncated file: normal v3 preflight decides recovery.
        if (std::equal(magic.begin(), magic.end(), prefix.begin())) {
            const std::uint16_t version = std::uint16_t(prefix[8]) |
                                          (std::uint16_t(prefix[9]) << 8);
            if (!std::ranges::contains(expectedVersions, version))
                throw std::runtime_error("MM6 slot contains a foreign EC6 version");
        }
    }
}

void requireDisposableV4Destination(const std::filesystem::path &profileRoot,
                                    std::string_view slotName) {
    if ((slotName != "candidate-v4.ec6" &&
         slotName != "candidate-v4-after-action.ec6") ||
        !profileRoot.is_absolute() ||
        profileRoot.lexically_normal() != profileRoot ||
        std::filesystem::is_symlink(std::filesystem::symlink_status(profileRoot)))
        throw std::invalid_argument("MM6 v4 test writer requires its disposable slot");
    const std::filesystem::path canonicalRoot = std::filesystem::canonical(profileRoot);
    if (canonicalRoot.parent_path() !=
            std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !canonicalRoot.filename().string().starts_with("mm6-restart-") ||
        !std::filesystem::is_directory(std::filesystem::symlink_status(profileRoot)) ||
        (std::filesystem::status(profileRoot).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) !=
            std::filesystem::perms::none)
        throw std::invalid_argument("MM6 v4 test writer requires an owner-only temp profile");
    const std::filesystem::path saves = profileRoot / "saves";
    const auto savesStatus = std::filesystem::symlink_status(saves);
    if (savesStatus.type() != std::filesystem::file_type::not_found &&
        !std::filesystem::is_directory(savesStatus))
        throw std::invalid_argument("MM6 v4 test saves path is not a native directory");
}

void requireDisposableV5Destination(const std::filesystem::path &profileRoot,
                                    std::string_view slotName) {
    if (slotName != "candidate-v5-after-action.ec6" &&
        slotName != "candidate-v5-after-reload-input.ec6" &&
        slotName != "candidate-v5-oute2.ec6" &&
        slotName != "candidate-v5-oute2-after-input.ec6")
        throw std::invalid_argument("MM6 v5 test writer requires its own candidate slot");
    // Same owner-only, direct system-temp profile proof as the v4 route.
    requireDisposableV4Destination(profileRoot, "candidate-v4.ec6");
}

void requireV5FirstMapPolicyInput(GameTitle title, std::string_view slotName,
                                  const SaveCandidateValidator &validate) {
    if (title != GameTitle::MM6 ||
        (slotName != "candidate-v5-after-action.ec6" &&
         slotName != "candidate-v5-after-reload-input.ec6"))
        throw std::invalid_argument("MM6 first-map v5 policy requires its title and slot");
    if (!validate)
        throw std::invalid_argument("MM6 first-map v5 policy needs semantic validation");
}

void validateV5FirstMapCandidate(std::span<const std::uint8_t> bytes,
                                 const SaveCandidateValidator &validate) {
    if (decodeV5SlotShell(bytes).mapFileName != "oute3.odm")
        throw std::runtime_error("MM6 first-map v5 slot has another map");
    validate(bytes);
}

void validateNumberedCandidate(std::span<const std::uint8_t> bytes,
                               const SaveCandidateValidator &validate) {
    if (decodeSaveContainer(bytes).version == 7) {
        (void) decodeV7SlotShell(bytes);
        validate(bytes);
    } else {
        validateV5FirstMapCandidate(bytes, validate);
    }
}

void requireDisposableV6OutdoorDestination(
    const std::filesystem::path &profileRoot, std::string_view slotName) {
    if (slotName != "candidate-v6-outdoor.ec6")
        throw std::invalid_argument("MM6 outdoor v6 test writer requires its own slot");
    requireDisposableV4Destination(profileRoot, "candidate-v4.ec6");
}

std::optional<std::string> slotNameFromFile(std::string name) {
    if (name.ends_with(".ec6.bak"))
        name.resize(name.size() - 4);
    if (!name.ends_with(".ec6"))
        return std::nullopt;
    try {
        validateSlot(name);
    } catch (const std::invalid_argument &) {
        return std::nullopt;
    }
    return name;
}

#ifndef _WIN32

void syncDirectory(const std::filesystem::path &directory) {
    int fd = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd < 0)
        throw std::system_error(errno, std::generic_category(), "open MM6 save directory");
    int result = ::fsync(fd);
    int error = errno;
    ::close(fd);
    if (result < 0)
        throw std::system_error(error, std::generic_category(), "sync MM6 save directory");
}

class TempSaveFile {
 public:
    TempSaveFile(const std::filesystem::path &directory, std::string_view slot) {
        std::string pattern = (directory / ("." + std::string(slot) + ".tmp.XXXXXX")).string();
        _fd = ::mkstemp(pattern.data());
        if (_fd < 0)
            throw std::system_error(errno, std::generic_category(), "create MM6 save temp");
        _path = std::move(pattern);
    }

    ~TempSaveFile() {
        if (_fd >= 0)
            ::close(_fd);
        if (!_path.empty())
            ::unlink(_path.c_str());
    }

    TempSaveFile(const TempSaveFile &) = delete;
    TempSaveFile &operator=(const TempSaveFile &) = delete;

    void write(std::span<const std::uint8_t> bytes,
               SlotSchema schema, const detail::AtomicSaveFault &fault = {}) {
        size_t offset = 0;
        while (offset < bytes.size()) {
            const size_t count = fault && offset == 0 ? 1 : bytes.size() - offset;
            ssize_t result = ::write(_fd, bytes.data() + offset, count);
            if (result < 0 && errno == EINTR)
                continue;
            if (result <= 0)
                throw std::system_error(result < 0 ? errno : EIO, std::generic_category(),
                                        "write MM6 save temp");
            offset += static_cast<size_t>(result);
            if (fault && offset == 1)
                fault(detail::AtomicSaveStep::AfterPartialTempWrite);
        }
        if (::fsync(_fd) < 0)
            throw std::system_error(errno, std::generic_category(), "sync MM6 save temp");
        if (::close(_fd) < 0) {
            _fd = -1;
            throw std::system_error(errno, std::generic_category(), "close MM6 save temp");
        }
        _fd = -1;
        if (readValidatedFile(_path, schema) != std::vector<std::uint8_t>(bytes.begin(), bytes.end()))
            throw std::runtime_error("MM6 save temp readback mismatch");
    }

    void replace(const std::filesystem::path &destination) {
        if (::rename(_path.c_str(), destination.c_str()) < 0)
            throw std::system_error(errno, std::generic_category(), "replace MM6 save file");
        _path.clear();
    }

 private:
    std::filesystem::path _path;
    int _fd = -1;
};

void restoreBackup(const std::filesystem::path &directory, std::string_view slot,
                   std::span<const std::uint8_t> bytes, SlotSchema schema) {
    TempSaveFile temp(directory, slot);
    temp.write(bytes, schema);
    temp.replace(directory / std::string(slot));
    syncDirectory(directory);
}

#endif

} // namespace

static void writeAtomicSaveFileForSchema(const std::filesystem::path &profileRoot,
                                         std::string_view slotName,
                                         std::span<const std::uint8_t> container,
                                         SlotSchema schema,
                                         const detail::AtomicSaveFault &fault) {
#ifdef _WIN32
    throw std::runtime_error("native atomic MM6 saves are unsupported on Windows");
#else
    if (schema == SlotSchema::KnownV1)
        (void) decodeKnownSave(container);
    else if (schema == SlotSchema::RawV2)
        (void) decodeRawSave(container, GameTitle::MM6);
    else if (schema == SlotSchema::FullV3)
        (void) decodeV3SlotShell(container);
    else if (schema == SlotSchema::FullV4)
        (void) decodeV4SlotShell(container);
    else if (schema == SlotSchema::FullV5)
        (void) decodeV5SlotShell(container);
    else if (schema == SlotSchema::NumberedV5OrV7)
        (void) decodeNumberedSlotShell(container);
    else
        (void) decodeV6OutdoorSlotShell(container);
    const std::filesystem::path directory = savesDirectory(profileRoot, slotName);
    std::filesystem::create_directories(directory);
    const std::filesystem::path primary = directory / std::string(slotName);
    const std::filesystem::path backup = directory / (std::string(slotName) + ".bak");

    std::optional<std::vector<std::uint8_t>> previous;
    if (regularFileExists(primary)) {
        previous = readValidatedFile(primary, schema);
    } else if (regularFileExists(backup)) {
        throw std::runtime_error("recover MM6 save backup before writing slot");
    }

    TempSaveFile next(directory, slotName);
    if (fault)
        fault(detail::AtomicSaveStep::BeforeTempWrite);
    next.write(container, schema, fault);

    if (previous) {
        TempSaveFile prior(directory, slotName);
        prior.write(*previous, schema);
        if (fault)
            fault(detail::AtomicSaveStep::BeforeBackupReplace);
        prior.replace(backup);
        syncDirectory(directory);
    }

    if (fault)
        fault(detail::AtomicSaveStep::BeforePrimaryReplace);
    next.replace(primary);
    if (fault)
        fault(detail::AtomicSaveStep::AfterPrimaryReplaceBeforeDirectorySync);
    syncDirectory(directory);
#endif
}

void detail::writeAtomicSaveFile(const std::filesystem::path &profileRoot,
                                 std::string_view slotName,
                                 std::span<const std::uint8_t> container,
                                 const AtomicSaveFault &fault) {
    writeAtomicSaveFileForSchema(profileRoot, slotName, container, SlotSchema::KnownV1, fault);
}

void detail::writeAtomicRawSaveFile(const std::filesystem::path &profileRoot,
                                    std::string_view slotName,
                                    std::span<const std::uint8_t> container,
                                    const AtomicSaveFault &fault) {
    writeAtomicSaveFileForSchema(profileRoot, slotName, container, SlotSchema::RawV2, fault);
}

void writeAtomicSaveFile(const std::filesystem::path &profileRoot, std::string_view slotName,
                         std::span<const std::uint8_t> container) {
    detail::writeAtomicSaveFile(profileRoot, slotName, container, {});
}

void writeAtomicRawSaveFile(const std::filesystem::path &profileRoot, std::string_view slotName,
                            std::span<const std::uint8_t> container) {
    detail::writeAtomicRawSaveFile(profileRoot, slotName, container, {});
}

void detail::writeAtomicV3SaveFile(const std::filesystem::path &profileRoot,
                                   std::string_view slotName,
                                   std::span<const std::uint8_t> container,
                                   const SaveCandidateValidator &validate,
                                   const AtomicSaveFault &fault) {
    if (!validate)
        throw std::invalid_argument("MM6 v3 write needs a semantic validator");
    (void) decodeV3SlotShell(container);
    validate(container);
    rejectForeignSlotFiles(profileRoot, slotName, {3});
    writeAtomicSaveFileForSchema(profileRoot, slotName, container, SlotSchema::FullV3, fault);
}

void writeAtomicV3SaveFile(const std::filesystem::path &profileRoot,
                           std::string_view slotName,
                           std::span<const std::uint8_t> container,
                           const SaveCandidateValidator &validate) {
    detail::writeAtomicV3SaveFile(profileRoot, slotName, container, validate, {});
}

void detail::writeAtomicV4SaveFileForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    std::span<const std::uint8_t> container,
    const SaveCandidateValidator &validate, const AtomicSaveFault &fault) {
    if (!validate)
        throw std::invalid_argument("MM6 v4 test write needs a semantic validator");
    requireDisposableV4Destination(profileRoot, slotName);
    (void) decodeV4SlotShell(container);
    validate(container);
    rejectForeignSlotFiles(profileRoot, slotName, {4});
    const std::filesystem::path directory = savesDirectory(profileRoot, slotName);
    for (const std::filesystem::path &path : {
             directory / std::string(slotName),
             directory / (std::string(slotName) + ".bak")}) {
        if (regularFileExists(path))
            validate(readValidatedFile(path, SlotSchema::FullV4));
    }
    writeAtomicSaveFileForSchema(profileRoot, slotName, container,
                                 SlotSchema::FullV4, fault);
}

void writeAtomicV4SaveFileForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    std::span<const std::uint8_t> container,
    const SaveCandidateValidator &validate) {
    detail::writeAtomicV4SaveFileForTest(profileRoot, slotName, container,
                                         validate, {});
}

void detail::writeAtomicV5SaveFileForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    std::span<const std::uint8_t> container,
    const SaveCandidateValidator &validate, const AtomicSaveFault &fault) {
    if (!validate)
        throw std::invalid_argument("MM6 v5 test write needs a semantic validator");
    requireDisposableV5Destination(profileRoot, slotName);
    (void) decodeV5SlotShell(container);
    validate(container);
    rejectForeignSlotFiles(profileRoot, slotName, {5});
    const std::filesystem::path directory = savesDirectory(profileRoot, slotName);
    for (const std::filesystem::path &path : {
             directory / std::string(slotName),
             directory / (std::string(slotName) + ".bak")}) {
        if (regularFileExists(path))
            validate(readValidatedFile(path, SlotSchema::FullV5));
    }
    writeAtomicSaveFileForSchema(profileRoot, slotName, container,
                                 SlotSchema::FullV5, fault);
}

void writeAtomicV5SaveFileForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    std::span<const std::uint8_t> container,
    const SaveCandidateValidator &validate) {
    detail::writeAtomicV5SaveFileForTest(profileRoot, slotName, container,
                                         validate, {});
}

void detail::writeAtomicV5FirstMapNativeSaveFileForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    std::span<const std::uint8_t> container,
    const SaveCandidateValidator &validate, const AtomicSaveFault &fault) {
    if (!validate || slotName.size() != 11 ||
        !slotName.starts_with("save") || !slotName.ends_with(".ec6") ||
        !std::all_of(slotName.begin() + 4, slotName.begin() + 7,
                     [](char digit) { return digit >= '0' && digit <= '9'; }))
        throw std::invalid_argument("MM6 native first-map writer needs numbered slot and validator");
    validateNumberedCandidate(container, validate);
    rejectForeignSlotFiles(profileRoot, slotName, {5, 7});
    const auto directory = savesDirectory(profileRoot, slotName);
    for (const std::filesystem::path &path : {
             directory / std::string(slotName),
             directory / (std::string(slotName) + ".bak")}) {
        if (regularFileExists(path))
            validateNumberedCandidate(readValidatedFile(path, SlotSchema::NumberedV5OrV7),
                                      validate);
    }
    writeAtomicSaveFileForSchema(profileRoot, slotName, container,
                                 SlotSchema::NumberedV5OrV7, fault);
}

V5FirstMapWriteResult assessV5FirstMapWriteResultForTest(
    GameTitle title, std::string_view slotName,
    std::span<const std::uint8_t> proposed,
    const V5FirstMapSlotState &before, const V5FirstMapSlotState &after,
    bool writerReturned, const SaveCandidateValidator &validate) {
    requireV5FirstMapPolicyInput(title, slotName, validate);
    validateV5FirstMapCandidate(proposed, validate);
    const auto requireState = [&](const V5FirstMapSlotState &state) {
        if (state.primary)
            validateV5FirstMapCandidate(*state.primary, validate);
        if (state.backup)
            validateV5FirstMapCandidate(*state.backup, validate);
    };
    requireState(before);
    requireState(after);
    if (!before.primary && before.backup)
        throw std::runtime_error("MM6 v5 backup-only slot needs read-only preflight");

    const auto expectedBackup = before.primary ? before.primary : before.backup;
    if (after.backup != before.backup && after.backup != expectedBackup)
        throw std::runtime_error("MM6 v5 backup changed outside atomic policy");
    const bool candidateIsPrimary = after.primary &&
        std::ranges::equal(*after.primary, proposed);
    if (writerReturned) {
        if (!candidateIsPrimary || after.backup != expectedBackup)
            throw std::runtime_error("MM6 v5 successful writer has unexpected slot bytes");
        return V5FirstMapWriteResult::Committed;
    }
    if (after.primary == before.primary)
        return V5FirstMapWriteResult::NoNewPrimaryNeedsPreflight;
    if (candidateIsPrimary && after.backup == expectedBackup)
        return V5FirstMapWriteResult::NewPrimaryUnconfirmedNeedsPreflight;
    throw std::runtime_error("MM6 v5 failed writer has unexpected slot bytes");
}

void detail::writeAtomicV6OutdoorSaveFileForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    std::span<const std::uint8_t> container,
    const SaveCandidateValidator &validate, const AtomicSaveFault &fault) {
    if (!validate)
        throw std::invalid_argument("MM6 outdoor v6 write needs a semantic validator");
    requireDisposableV6OutdoorDestination(profileRoot, slotName);
    (void) decodeV6OutdoorSlotShell(container);
    validate(container);
    rejectForeignSlotFiles(profileRoot, slotName, {6});
    const std::filesystem::path directory = savesDirectory(profileRoot, slotName);
    for (const std::filesystem::path &path : {
             directory / std::string(slotName),
             directory / (std::string(slotName) + ".bak")}) {
        if (regularFileExists(path))
            validate(readValidatedFile(path, SlotSchema::FullV6Outdoor));
    }
    writeAtomicSaveFileForSchema(profileRoot, slotName, container,
                                 SlotSchema::FullV6Outdoor, fault);
}

void writeAtomicV6OutdoorSaveFileForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    std::span<const std::uint8_t> container,
    const SaveCandidateValidator &validate) {
    detail::writeAtomicV6OutdoorSaveFileForTest(profileRoot, slotName,
                                                container, validate, {});
}

detail::SaveSlotRollbackSnapshot detail::snapshotSaveSlotForRollback(
    const std::filesystem::path &profileRoot, std::string_view slotName) {
#ifdef _WIN32
    throw std::runtime_error("native MM6 save rollback is unsupported on Windows");
#else
    const std::filesystem::path directory = savesDirectory(profileRoot, slotName);
    const auto status = std::filesystem::symlink_status(directory);
    if (status.type() == std::filesystem::file_type::not_found)
        return {};
    if (!std::filesystem::is_directory(status))
        throw std::runtime_error("MM6 saves path is not a native directory");
    const std::filesystem::path primary = directory / std::string(slotName);
    const std::filesystem::path backup = directory / (std::string(slotName) + ".bak");
    SaveSlotRollbackSnapshot snapshot;
    if (regularFileExists(primary))
        snapshot.primary = readBoundedFile(primary);
    if (regularFileExists(backup))
        snapshot.backup = readBoundedFile(backup);
    return snapshot;
#endif
}

void detail::restoreSaveSlotForRollback(const std::filesystem::path &profileRoot,
                                        std::string_view slotName,
                                        const SaveSlotRollbackSnapshot &snapshot) {
#ifdef _WIN32
    throw std::runtime_error("native MM6 save rollback is unsupported on Windows");
#else
    const std::filesystem::path directory = savesDirectory(profileRoot, slotName);
    const auto status = std::filesystem::symlink_status(directory);
    if (status.type() == std::filesystem::file_type::not_found &&
        !snapshot.primary && !snapshot.backup)
        return;
    if (!std::filesystem::is_directory(status))
        throw std::runtime_error("MM6 saves path is not a native directory");
    const auto restoreOne = [&](const std::filesystem::path &path,
                                const std::optional<std::vector<std::uint8_t>> &bytes) {
        if (bytes) {
            TempSaveFile temp(directory, slotName);
            temp.write(*bytes, SlotSchema::KnownV1);
            temp.replace(path);
        } else if (regularFileExists(path)) {
            std::filesystem::remove(path);
        }
    };
    restoreOne(directory / std::string(slotName), snapshot.primary);
    restoreOne(directory / (std::string(slotName) + ".bak"), snapshot.backup);
    syncDirectory(directory);
#endif
}

static std::optional<std::vector<std::uint8_t>> loadRecoverableSaveFileForSchema(
    const std::filesystem::path &profileRoot, std::string_view slotName, SlotSchema schema) {
#ifdef _WIN32
    throw std::runtime_error("native atomic MM6 saves are unsupported on Windows");
#else
    const std::filesystem::path directory = savesDirectory(profileRoot, slotName);
    const std::filesystem::path primary = directory / std::string(slotName);
    const std::filesystem::path backup = directory / (std::string(slotName) + ".bak");
    if (regularFileExists(primary)) {
        try {
            return readValidatedFile(primary, schema);
        } catch (const InvalidSaveFile &) {
            if (!regularFileExists(backup))
                throw;
        }
    } else if (!regularFileExists(backup)) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> recovered = readValidatedFile(backup, schema);
    restoreBackup(directory, slotName, recovered, schema);
    return recovered;
#endif
}

std::optional<std::vector<std::uint8_t>> loadRecoverableSaveFile(
    const std::filesystem::path &profileRoot, std::string_view slotName) {
    return loadRecoverableSaveFileForSchema(profileRoot, slotName, SlotSchema::KnownV1);
}

std::optional<std::vector<std::uint8_t>> loadRecoverableRawSaveFile(
    const std::filesystem::path &profileRoot, std::string_view slotName) {
    return loadRecoverableSaveFileForSchema(profileRoot, slotName, SlotSchema::RawV2);
}

static std::optional<SaveFileCandidate> preflightRecoverableSaveFileForSchema(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    const SaveCandidateValidator &validate, SlotSchema schema) {
#ifdef _WIN32
    throw std::runtime_error("native MM6 save preflight is unsupported on Windows");
#else
    if (!validate)
        throw std::invalid_argument("MM6 save preflight needs a semantic validator");
    const std::filesystem::path directory = savesDirectory(profileRoot, slotName);
    const std::filesystem::file_status directoryStatus = std::filesystem::symlink_status(directory);
    if (directoryStatus.type() == std::filesystem::file_type::not_found)
        return std::nullopt;
    if (!std::filesystem::is_directory(directoryStatus))
        throw std::runtime_error("MM6 saves path is not a native directory");
    const std::filesystem::path primary = directory / std::string(slotName);
    const std::filesystem::path backup = directory / (std::string(slotName) + ".bak");
    bool foundCandidate = false;
    for (const std::filesystem::path &path : {primary, backup}) {
        if (!regularFileExists(path))
            continue;
        foundCandidate = true;
        std::vector<std::uint8_t> bytes;
        try {
            bytes = readValidatedFile(path, schema);
        } catch (const InvalidSaveFile &) {
            continue;
        }
        try {
            validate(bytes);
        } catch (const std::runtime_error &) {
            continue;
        }
        return SaveFileCandidate{std::move(bytes), path == backup};
    }
    if (foundCandidate)
        throw InvalidSaveFile("MM6 save slot has no semantically valid candidate");
    return std::nullopt;
#endif
}

std::optional<SaveFileCandidate> preflightRecoverableSaveFile(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    const SaveCandidateValidator &validate) {
    return preflightRecoverableSaveFileForSchema(profileRoot, slotName, validate,
                                                 SlotSchema::KnownV1);
}

std::optional<SaveFileCandidate> preflightRecoverableRawSaveFile(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    const SaveCandidateValidator &validate) {
    return preflightRecoverableSaveFileForSchema(profileRoot, slotName, validate,
                                                 SlotSchema::RawV2);
}

std::optional<SaveFileCandidate> preflightRecoverableV3SaveFile(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    const SaveCandidateValidator &validate) {
    if (!validate)
        throw std::invalid_argument("MM6 v3 preflight needs a semantic validator");
    rejectForeignSlotFiles(profileRoot, slotName, {3});
    return preflightRecoverableSaveFileForSchema(profileRoot, slotName, validate,
                                                 SlotSchema::FullV3);
}

std::optional<SaveFileCandidate> preflightRecoverableV5FirstMapSaveFileForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    GameTitle title, const SaveCandidateValidator &validate) {
    requireV5FirstMapPolicyInput(title, slotName, validate);
    requireDisposableV5Destination(profileRoot, slotName);
    rejectForeignSlotFiles(profileRoot, slotName, {5});
    const auto directory = savesDirectory(profileRoot, slotName);
    for (const std::filesystem::path &path : {
             directory / std::string(slotName),
             directory / (std::string(slotName) + ".bak")}) {
        if (!regularFileExists(path))
            continue;
        SaveHeaderFields header;
        try {
            (void) readValidatedFile(path, SlotSchema::FullV5, &header);
        } catch (const InvalidSaveFile &) {
            continue; // A damaged candidate may fall back to a valid backup.
        }
        if (header.mapFileName != "oute3.odm")
            throw std::runtime_error("MM6 first-map v5 slot contains another map");
    }
    return preflightRecoverableSaveFileForSchema(
        profileRoot, slotName,
        [&](std::span<const std::uint8_t> bytes) {
            validateV5FirstMapCandidate(bytes, validate);
        },
        SlotSchema::FullV5);
}

std::optional<SaveFileCandidate> preflightRecoverableV6OutdoorSaveFileForTest(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    const SaveCandidateValidator &validate) {
    if (!validate)
        throw std::invalid_argument("MM6 outdoor v6 preflight needs semantic validation");
    requireDisposableV6OutdoorDestination(profileRoot, slotName);
    rejectForeignSlotFiles(profileRoot, slotName, {6});
    return preflightRecoverableSaveFileForSchema(
        profileRoot, slotName, validate, SlotSchema::FullV6Outdoor);
}

static std::vector<SaveSlotInfo> listSaveSlotsForSchema(const std::filesystem::path &profileRoot,
                                                        SlotSchema schema) {
#ifdef _WIN32
    throw std::runtime_error("native MM6 save slots are unsupported on Windows");
#else
    const std::filesystem::path directory = savesDirectory(profileRoot, "save000.ec6");
    const std::filesystem::file_status directoryStatus = std::filesystem::symlink_status(directory);
    if (directoryStatus.type() == std::filesystem::file_type::not_found)
        return {};
    if (!std::filesystem::is_directory(directoryStatus))
        throw std::runtime_error("MM6 saves path is not a native directory");

    std::set<std::string> names;
    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(directory)) {
        if (std::optional<std::string> name = slotNameFromFile(entry.path().filename().string()))
            names.insert(std::move(*name));
    }

    std::vector<SaveSlotInfo> slots;
    for (const std::string &name : names) {
        const std::filesystem::path primary = directory / name;
        const std::filesystem::path backup = directory / (name + ".bak");
        SaveHeaderFields header;
        if (regularFileExists(primary)) {
            try {
                (void) readValidatedFile(primary, schema, &header);
                slots.push_back({name, std::move(header), false});
                continue;
            } catch (const InvalidSaveFile &) {
                // A valid backup may still recover this slot.
            }
        }
        if (regularFileExists(backup)) {
            try {
                (void) readValidatedFile(backup, schema, &header);
                slots.push_back({name, std::move(header), true});
            } catch (const InvalidSaveFile &) {
                // Do not present an unusable slot as playable.
            }
        }
    }
    return slots;
#endif
}

std::vector<SaveSlotInfo> listKnownSaveSlots(const std::filesystem::path &profileRoot) {
    return listSaveSlotsForSchema(profileRoot, SlotSchema::KnownV1);
}

std::vector<SaveSlotInfo> listRawSaveSlots(const std::filesystem::path &profileRoot) {
    return listSaveSlotsForSchema(profileRoot, SlotSchema::RawV2);
}

std::vector<SaveSlotInfo> listV3SaveSlotsWithPreflight(
    const std::filesystem::path &profileRoot, const SaveCandidateValidator &validate) {
    if (!validate)
        throw std::invalid_argument("MM6 v3 slot list needs a semantic validator");
    std::vector<SaveSlotInfo> result;
    for (const SaveSlotInfo &slot : listSaveSlotsForSchema(profileRoot, SlotSchema::FullV3)) {
        try {
            auto candidate = preflightRecoverableV3SaveFile(profileRoot, slot.fileName, validate);
            if (!candidate)
                continue;
            SaveHeaderFields header = decodeV3SlotShell(candidate->bytes);
            result.push_back({slot.fileName, std::move(header),
                              candidate->needsRecovery});
        } catch (const InvalidSaveFile &) {
            // A structural candidate that fails gameplay preflight stays hidden.
        }
    }
    return result;
}

std::vector<SaveSlotInfo> listRawSaveSlotsWithPreflight(
    const std::filesystem::path &profileRoot, const SaveCandidateValidator &validate) {
    if (!validate)
        throw std::invalid_argument("MM6 save menu needs a semantic validator");
    std::vector<SaveSlotInfo> result;
    for (const SaveSlotInfo &slot : listRawSaveSlots(profileRoot)) {
        try {
            auto candidate = preflightRecoverableRawSaveFile(profileRoot, slot.fileName, validate);
            if (!candidate)
                continue;
            RawSaveFields fields = decodeRawSave(candidate->bytes, GameTitle::MM6);
            result.push_back({slot.fileName, std::move(fields.metadata.header),
                              candidate->needsRecovery});
        } catch (const InvalidSaveFile &) {
            // Structural slots with no semantically admissible candidate stay hidden.
        }
    }
    return result;
}

} // namespace mm6
