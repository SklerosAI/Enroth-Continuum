#include "MM6AtomicSaveFile.h"

#include <cerrno>
#include <cstring>
#include <fstream>
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

namespace mm6 {
namespace {

constexpr std::uintmax_t MAX_SAVE_BYTES = 64 * 1024 * 1024;

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
                                            SaveHeaderFields *header = nullptr) {
    std::vector<std::uint8_t> bytes = readBoundedFile(path);
    try {
        KnownSaveFields fields = decodeKnownSave(bytes);
        if (header)
            *header = std::move(fields.metadata.header);
    } catch (const std::runtime_error &) {
        throw InvalidSaveFile("MM6 known save validation failed");
    }
    return bytes;
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
               const detail::AtomicSaveFault &fault = {}) {
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
        if (readValidatedFile(_path) != std::vector<std::uint8_t>(bytes.begin(), bytes.end()))
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
                   std::span<const std::uint8_t> bytes) {
    TempSaveFile temp(directory, slot);
    temp.write(bytes);
    temp.replace(directory / std::string(slot));
    syncDirectory(directory);
}

#endif

} // namespace

void detail::writeAtomicSaveFile(const std::filesystem::path &profileRoot,
                                 std::string_view slotName,
                                 std::span<const std::uint8_t> container,
                                 const AtomicSaveFault &fault) {
#ifdef _WIN32
    throw std::runtime_error("native atomic MM6 saves are unsupported on Windows");
#else
    (void) decodeKnownSave(container);
    const std::filesystem::path directory = savesDirectory(profileRoot, slotName);
    std::filesystem::create_directories(directory);
    const std::filesystem::path primary = directory / std::string(slotName);
    const std::filesystem::path backup = directory / (std::string(slotName) + ".bak");

    std::optional<std::vector<std::uint8_t>> previous;
    if (regularFileExists(primary)) {
        previous = readValidatedFile(primary);
    } else if (regularFileExists(backup)) {
        throw std::runtime_error("recover MM6 save backup before writing slot");
    }

    TempSaveFile next(directory, slotName);
    if (fault)
        fault(AtomicSaveStep::BeforeTempWrite);
    next.write(container, fault);

    if (previous) {
        TempSaveFile prior(directory, slotName);
        prior.write(*previous);
        if (fault)
            fault(AtomicSaveStep::BeforeBackupReplace);
        prior.replace(backup);
        syncDirectory(directory);
    }

    if (fault)
        fault(AtomicSaveStep::BeforePrimaryReplace);
    next.replace(primary);
    syncDirectory(directory);
#endif
}

void writeAtomicSaveFile(const std::filesystem::path &profileRoot, std::string_view slotName,
                         std::span<const std::uint8_t> container) {
    detail::writeAtomicSaveFile(profileRoot, slotName, container, {});
}

std::optional<std::vector<std::uint8_t>> loadRecoverableSaveFile(
    const std::filesystem::path &profileRoot, std::string_view slotName) {
#ifdef _WIN32
    throw std::runtime_error("native atomic MM6 saves are unsupported on Windows");
#else
    const std::filesystem::path directory = savesDirectory(profileRoot, slotName);
    const std::filesystem::path primary = directory / std::string(slotName);
    const std::filesystem::path backup = directory / (std::string(slotName) + ".bak");
    if (regularFileExists(primary)) {
        try {
            return readValidatedFile(primary);
        } catch (const InvalidSaveFile &) {
            if (!regularFileExists(backup))
                throw;
        }
    } else if (!regularFileExists(backup)) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> recovered = readValidatedFile(backup);
    restoreBackup(directory, slotName, recovered);
    return recovered;
#endif
}

std::optional<SaveFileCandidate> preflightRecoverableSaveFile(
    const std::filesystem::path &profileRoot, std::string_view slotName,
    const SaveCandidateValidator &validate) {
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
            bytes = readValidatedFile(path);
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
        throw std::runtime_error("MM6 save slot has no semantically valid candidate");
    return std::nullopt;
#endif
}

std::vector<SaveSlotInfo> listKnownSaveSlots(const std::filesystem::path &profileRoot) {
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
                (void) readValidatedFile(primary, &header);
                slots.push_back({name, std::move(header), false});
                continue;
            } catch (const InvalidSaveFile &) {
                // A valid backup may still recover this slot.
            }
        }
        if (regularFileExists(backup)) {
            try {
                (void) readValidatedFile(backup, &header);
                slots.push_back({name, std::move(header), true});
            } catch (const InvalidSaveFile &) {
                // Do not present an unusable slot as playable.
            }
        }
    }
    return slots;
#endif
}

} // namespace mm6
