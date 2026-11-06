#include "MM6HeadlessRules.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "Library/FileSystem/Interface/FileSystem.h"

#include "Utility/Exception.h"

namespace {
constexpr std::int64_t MAX_RULE_BYTES = 1024 * 1024;
} // namespace

void seedMm6HeadlessRules(const FileSystem &source, FileSystem *destination) {
    if (!destination)
        throw Exception("MM6 headless rule destination is missing.");

    // Preflight all eight whitelisted files before writing any into ramfs.
    std::array<Blob, mm6HeadlessRuleNames.size()> staged;
    for (size_t i = 0; i < mm6HeadlessRuleNames.size(); ++i) {
        std::string_view name = mm6HeadlessRuleNames[i];
        FileStat stat = source.stat(name);
        if (stat.type != FILE_REGULAR || stat.size < 1 || stat.size > MAX_RULE_BYTES)
            throw Exception("MM6 headless rule '{}' is missing or outside the size limit.", name);
        staged[i] = source.read(name);
        if (staged[i].size() != static_cast<size_t>(stat.size))
            throw Exception("MM6 headless rule '{}' changed during reading.", name);
    }

    for (size_t i = 0; i < mm6HeadlessRuleNames.size(); ++i)
        destination->write("rules/" + std::string(mm6HeadlessRuleNames[i]), staged[i]);
}
