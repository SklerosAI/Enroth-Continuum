#include "MM7AtomicSaveFile.h"

#include <cerrno>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

namespace mm7 {
namespace {

constexpr std::uintmax_t MAX_SAVE_BYTES = 64 * 1024 * 1024;
std::optional<std::filesystem::path> startupRoot;

class InvalidSaveCandidate : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

void validateSlotName(std::string_view slotName) {
  if (slotName.size() <= 4 || slotName.size() > 255 ||
      !slotName.ends_with(".mm7"))
    throw std::invalid_argument("invalid MM7 save slot name");
  for (char c : slotName) {
    if (c == '/' || c == '\\' || c == '\0')
      throw std::invalid_argument("invalid MM7 save slot name");
  }
}

std::filesystem::path savesDirectory(const std::filesystem::path &profileRoot,
                                     std::string_view slotName) {
  validateSlotName(slotName);
  if (!profileRoot.is_absolute() || profileRoot.empty() ||
      !std::filesystem::is_directory(
          std::filesystem::symlink_status(profileRoot)))
    throw std::invalid_argument("MM7 profile root must be a native directory");
  return profileRoot / "saves";
}

bool regularFileExists(const std::filesystem::path &path) {
  const std::filesystem::file_status status =
      std::filesystem::symlink_status(path);
  if (status.type() == std::filesystem::file_type::not_found)
    return false;
  if (!std::filesystem::is_regular_file(status))
    throw std::runtime_error("MM7 save path is not a regular file");
  return true;
}

bool savesDirectoryExists(const std::filesystem::path &directory) {
  const std::filesystem::file_status status =
      std::filesystem::symlink_status(directory);
  if (status.type() == std::filesystem::file_type::not_found)
    return false;
  if (!std::filesystem::is_directory(status))
    throw std::runtime_error("MM7 saves path is not a native directory");
  return true;
}

std::vector<std::uint8_t> readBoundedFile(const std::filesystem::path &path) {
  const std::uintmax_t fileSize = std::filesystem::file_size(path);
  if (fileSize > MAX_SAVE_BYTES)
    throw InvalidSaveCandidate("MM7 save file exceeds size limit");
  std::ifstream input(path, std::ios::binary);
  if (!input)
    throw std::runtime_error("cannot open MM7 save file");
  const size_t size = static_cast<size_t>(fileSize);
  std::vector<std::uint8_t> bytes(size);
  if (size && !input.read(reinterpret_cast<char *>(bytes.data()), size))
    throw InvalidSaveCandidate("cannot read complete MM7 save file");
  if (input.peek() != std::char_traits<char>::eof())
    throw InvalidSaveCandidate("MM7 save file changed during read");
  return bytes;
}

std::vector<std::uint8_t>
readValidatedFile(const std::filesystem::path &path,
                  const SaveBytesValidator &validate) {
  std::vector<std::uint8_t> bytes = readBoundedFile(path);
  if (bytes.empty() || !validate(bytes))
    throw InvalidSaveCandidate("MM7 save content validation failed");
  return bytes;
}

#ifndef _WIN32
void syncDirectory(const std::filesystem::path &directory) {
  const int fd = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY);
  if (fd < 0)
    throw std::system_error(errno, std::generic_category(),
                            "open MM7 save directory");
  const int result = ::fsync(fd);
  const int error = errno;
  ::close(fd);
  if (result < 0)
    throw std::system_error(error, std::generic_category(),
                            "sync MM7 save directory");
}

class TempSaveFile {
 public:
  TempSaveFile(const std::filesystem::path &directory,
               std::string_view slotName) {
    std::string pattern =
        (directory / ("." + std::string(slotName) + ".tmp.XXXXXX")).string();
    _fd = ::mkstemp(pattern.data());
    if (_fd < 0)
      throw std::system_error(errno, std::generic_category(),
                              "create MM7 save temp");
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
             const SaveBytesValidator &validate,
             const detail::AtomicSaveFault &fault = {}) {
    size_t offset = 0;
    while (offset < bytes.size()) {
      const size_t count = fault && offset == 0 ? 1 : bytes.size() - offset;
      const ssize_t result = ::write(_fd, bytes.data() + offset, count);
      if (result < 0 && errno == EINTR)
        continue;
      if (result <= 0)
        throw std::system_error(result < 0 ? errno : EIO,
                                std::generic_category(), "write MM7 save temp");
      offset += static_cast<size_t>(result);
      if (fault && offset == 1)
        fault(detail::AtomicSaveStep::AfterPartialTempWrite);
    }
    if (::fsync(_fd) < 0)
      throw std::system_error(errno, std::generic_category(),
                              "sync MM7 save temp");
    if (::close(_fd) < 0) {
      _fd = -1;
      throw std::system_error(errno, std::generic_category(),
                              "close MM7 save temp");
    }
    _fd = -1;
    if (readValidatedFile(_path, validate) !=
        std::vector<std::uint8_t>(bytes.begin(), bytes.end()))
      throw std::runtime_error("MM7 save temp readback mismatch");
  }

  void replace(const std::filesystem::path &destination) {
    if (::rename(_path.c_str(), destination.c_str()) < 0)
      throw std::system_error(errno, std::generic_category(),
                              "replace MM7 save file");
    _path.clear();
  }

 private:
  std::filesystem::path _path;
  int _fd = -1;
};
#endif

} // namespace

void setStartupNativeProfileRoot(GameTitle title, bool ramFsUserData,
                                 const std::filesystem::path &userPath) {
  startupRoot.reset();
  if (title != GameTitle::MM7 || ramFsUserData || userPath.empty() ||
      !userPath.is_absolute() || userPath != userPath.lexically_normal())
    return;

  std::error_code error;
  const std::filesystem::file_status status =
      std::filesystem::symlink_status(userPath, error);
  if ((error && error != std::errc::no_such_file_or_directory) ||
      std::filesystem::is_symlink(status) ||
      (std::filesystem::exists(status) && !std::filesystem::is_directory(status)))
    return;

  const std::filesystem::path canonical =
      std::filesystem::weakly_canonical(userPath, error);
  if (!error && canonical.is_absolute())
    startupRoot = canonical;
}

std::optional<std::filesystem::path> startupNativeProfileRoot() {
  if (!startupRoot)
    return std::nullopt;
  std::error_code error;
  const std::filesystem::file_status status =
      std::filesystem::symlink_status(*startupRoot, error);
  if (error || !std::filesystem::is_directory(status))
    return std::nullopt;
  const std::filesystem::path canonical =
      std::filesystem::canonical(*startupRoot, error);
  if (error || canonical != *startupRoot)
    return std::nullopt;
  return startupRoot;
}

std::optional<SaveFileCandidate>
preflightSaveSlot(const std::filesystem::path &profileRoot,
                  std::string_view slotName,
                  const SaveBytesValidator &validate) {
  if (!validate)
    throw std::invalid_argument(
        "MM7 save preflight requires content validation");
  const std::filesystem::path directory = savesDirectory(profileRoot, slotName);
  if (!savesDirectoryExists(directory))
    return std::nullopt;
  const std::filesystem::path primary = directory / std::string(slotName);
  const std::filesystem::path backup =
      directory / (std::string(slotName) + ".bak");
  bool foundCandidate = false;
  for (const std::filesystem::path &path : {primary, backup}) {
    if (!regularFileExists(path))
      continue;
    foundCandidate = true;
    try {
      return SaveFileCandidate{readValidatedFile(path, validate),
                               path == backup};
    } catch (const InvalidSaveCandidate &) {
      // A damaged primary may still have an intact backup. Keep both files
      // untouched.
    }
  }
  if (foundCandidate)
    throw InvalidSaveCandidate("MM7 save slot has no valid primary or backup");
  return std::nullopt;
}

void detail::writeAtomicSaveSlot(const std::filesystem::path &profileRoot,
                                 std::string_view slotName,
                                 std::span<const std::uint8_t> bytes,
                                 const SaveBytesValidator &validate,
                                 const AtomicSaveFault &fault) {
#ifdef _WIN32
  throw std::runtime_error(
      "native atomic MM7 saves are unsupported on Windows");
#else
  if (!validate || bytes.empty() || bytes.size() > MAX_SAVE_BYTES ||
      !validate(bytes))
    throw std::invalid_argument("invalid MM7 save candidate");
  const std::filesystem::path directory = savesDirectory(profileRoot, slotName);
  const bool directoryExists = savesDirectoryExists(directory);
  const std::filesystem::path primary = directory / std::string(slotName);
  const std::filesystem::path backup =
      directory / (std::string(slotName) + ".bak");
  std::optional<std::vector<std::uint8_t>> previous;
  if (directoryExists && regularFileExists(primary)) {
    previous = readValidatedFile(primary, validate);
    if (regularFileExists(backup))
      (void)readValidatedFile(backup, validate);
  } else if (directoryExists && regularFileExists(backup)) {
    throw std::runtime_error("recover MM7 save backup before writing slot");
  }
  if (!directoryExists) {
    std::filesystem::create_directory(directory);
    syncDirectory(profileRoot); // Persist saves/ in its parent before the first slot.
  }
  TempSaveFile next(directory, slotName);
  if (fault)
    fault(AtomicSaveStep::BeforeTempWrite);
  next.write(bytes, validate, fault);
  if (previous) {
    TempSaveFile prior(directory, slotName);
    prior.write(*previous, validate);
    if (fault)
      fault(AtomicSaveStep::BeforeBackupReplace);
    prior.replace(backup);
    syncDirectory(directory);
  }
  if (fault)
    fault(AtomicSaveStep::BeforePrimaryReplace);
  next.replace(primary);
  if (fault)
    fault(AtomicSaveStep::AfterPrimaryReplaceBeforeDirectorySync);
  syncDirectory(directory);
#endif
}

void writeAtomicSaveSlot(const std::filesystem::path &profileRoot,
                         std::string_view slotName,
                         std::span<const std::uint8_t> bytes,
                         const SaveBytesValidator &validate) {
  detail::writeAtomicSaveSlot(profileRoot, slotName, bytes, validate, {});
}

} // namespace mm7
