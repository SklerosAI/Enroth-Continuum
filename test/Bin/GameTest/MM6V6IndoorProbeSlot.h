#pragma once

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#if !defined(_WIN32)
#include <fcntl.h>
#include <unistd.h>
#endif

namespace mm6::testing {

inline constexpr std::string_view V6_INDOOR_PROBE_SLOT = "candidate-v6-indoor.ec6";
inline constexpr std::string_view V6_INDOOR_POSTWORLD_PROBE_SLOT =
    "candidate-v6-indoor-postworld.ec6";
inline constexpr std::string_view V6_INDOOR_LIVE_SESSION_PROBE_SLOT =
    "candidate-v6-indoor-live-session.ec6";

inline std::filesystem::path indoorProbeProfile() {
    const char *value = std::getenv("ENROTH_MM6_RESTART_PROFILE");
    if (!value || !*value)
        return {};
    const std::filesystem::path supplied(value);
    if (!supplied.is_absolute() || !std::filesystem::is_directory(supplied))
        throw std::runtime_error("MM6 v6 indoor probe needs an absolute disposable profile");
    const auto root = std::filesystem::canonical(supplied);
    if (root.parent_path() != std::filesystem::canonical(std::filesystem::temp_directory_path()) ||
        !root.filename().string().starts_with("mm6-restart-") ||
        (std::filesystem::status(root).permissions() &
         (std::filesystem::perms::group_all | std::filesystem::perms::others_all)) !=
            std::filesystem::perms::none)
        throw std::runtime_error("MM6 v6 indoor probe needs an owner-only system-temp profile");
    const auto saves = root / "saves";
    if (!std::filesystem::is_directory(saves) || std::filesystem::is_symlink(saves) ||
        std::filesystem::canonical(saves).parent_path() != root)
        throw std::runtime_error("MM6 v6 indoor probe needs a local saves directory");
    return root;
}

inline std::vector<std::uint8_t> readIndoorProbeSlot(
    const std::filesystem::path &profile, std::string_view name) {
    if (name != "autosave.ec6" && name != V6_INDOOR_PROBE_SLOT &&
        name != V6_INDOOR_POSTWORLD_PROBE_SLOT &&
        name != V6_INDOOR_LIVE_SESSION_PROBE_SLOT)
        throw std::runtime_error("MM6 v6 indoor probe requested another slot");
    const auto path = profile / "saves" / name;
    if (!std::filesystem::is_regular_file(path) || std::filesystem::is_symlink(path) ||
        std::filesystem::file_size(path) > 64 * 1024 * 1024)
        throw std::runtime_error("MM6 v6 indoor probe needs a bounded slot");
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("MM6 v6 indoor probe slot cannot be read");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

// Test-only atomic write to a separate, disposable slot. The published bytes
// are already staged as a v6 candidate; no production save routing is used.
inline void writeIndoorProbeCandidate(const std::filesystem::path &profile,
                                      const std::vector<std::uint8_t> &bytes,
                                      std::string_view name = V6_INDOOR_PROBE_SLOT) {
#if defined(_WIN32)
    throw std::runtime_error("MM6 v6 disposable atomic probe requires POSIX");
#else
    const auto saves = profile / "saves";
    if (name != V6_INDOOR_PROBE_SLOT &&
        name != V6_INDOOR_POSTWORLD_PROBE_SLOT &&
        name != V6_INDOOR_LIVE_SESSION_PROBE_SLOT)
        throw std::runtime_error("MM6 v6 indoor probe refused an unknown slot");
    const auto primary = saves / name;
    const auto pending = saves / (std::string(name) + ".pending");
    if (bytes.empty() || std::filesystem::exists(primary) ||
        std::filesystem::exists(pending))
        throw std::runtime_error("MM6 v6 indoor probe slot is occupied or empty");
    const int fd = ::open(pending.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    if (fd < 0)
        throw std::runtime_error("MM6 v6 indoor probe cannot create pending slot");
    bool published = false;
    try {
        std::size_t offset = 0;
        while (offset < bytes.size()) {
            const ssize_t written = ::write(fd, bytes.data() + offset, bytes.size() - offset);
            if (written < 0 && errno == EINTR)
                continue;
            if (written <= 0)
                throw std::runtime_error("MM6 v6 indoor probe write failed");
            offset += written;
        }
        if (::fsync(fd) != 0)
            throw std::runtime_error("MM6 v6 indoor probe file sync failed");
        if (::close(fd) != 0)
            throw std::runtime_error("MM6 v6 indoor probe file close failed");
        if (::rename(pending.c_str(), primary.c_str()) != 0)
            throw std::runtime_error("MM6 v6 indoor probe atomic rename failed");
        published = true;
        const int directory = ::open(saves.c_str(), O_RDONLY);
        if (directory < 0 || ::fsync(directory) != 0) {
            if (directory >= 0) ::close(directory);
            throw std::runtime_error("MM6 v6 indoor probe directory sync failed");
        }
        ::close(directory);
    } catch (...) {
        if (!published) {
            ::close(fd);
            std::error_code ignored;
            std::filesystem::remove(pending, ignored);
        }
        throw;
    }
#endif
}

} // namespace mm6::testing
