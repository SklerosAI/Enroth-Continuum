#include <cstdint>
#include <exception>
#include <system_error>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifndef _WIN32
#include <cstdlib>
#endif

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM6/MM6AtomicSaveFile.h"
#include "Engine/MM6/MM6SaveContainer.h"

namespace {

class TestProfile {
 public:
    TestProfile() {
#ifndef _WIN32
        std::string pattern = (std::filesystem::temp_directory_path() / "mm6-atomic-XXXXXX").string();
        char *result = ::mkdtemp(pattern.data());
        if (!result)
            throw std::runtime_error("cannot create MM6 test profile");
        _root = result;
#endif
    }

    ~TestProfile() {
        if (!_root.empty()) {
            std::error_code error;
            std::filesystem::remove_all(_root, error);
        }
    }

    const std::filesystem::path &root() const { return _root; }
    std::filesystem::path save() const { return _root / "saves" / "save001.ec6"; }
    std::filesystem::path backup() const { return _root / "saves" / "save001.ec6.bak"; }

 private:
    std::filesystem::path _root;
};

std::vector<std::uint8_t> sample(std::uint8_t marker) {
    mm6::SaveContainer container;
    container.sections.push_back({{'H', 'E', 'A', 'D'}, true, {marker}});
    return mm6::encodeSaveContainer(container);
}

std::vector<std::uint8_t> read(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

} // namespace

#ifndef _WIN32
UNIT_TEST(MM6AtomicSaveFile, InitialReplaceAndRecoverPrevious) {
    TestProfile profile;
    std::vector<std::uint8_t> old = sample(1);
    std::vector<std::uint8_t> next = sample(2);
    EXPECT_EQ(mm6::loadRecoverableSaveFile(profile.root(), "save001.ec6"), std::nullopt);

    mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", old);
    EXPECT_EQ(read(profile.save()), old);
    EXPECT_FALSE(std::filesystem::exists(profile.backup()));

    mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", next);
    EXPECT_EQ(read(profile.save()), next);
    EXPECT_EQ(read(profile.backup()), old);

    { std::ofstream corrupt(profile.save(), std::ios::binary); corrupt << "bad"; }
    EXPECT_EQ(mm6::loadRecoverableSaveFile(profile.root(), "save001.ec6"), old);
    EXPECT_EQ(read(profile.save()), old);
    EXPECT_EQ(read(profile.backup()), old);

    std::filesystem::remove(profile.save());
    EXPECT_EQ(mm6::loadRecoverableSaveFile(profile.root(), "save001.ec6"), old);
    EXPECT_EQ(read(profile.save()), old);
}

UNIT_TEST(MM6AtomicSaveFile, FaultsPreservePreviousValidSave) {
    for (mm6::detail::AtomicSaveStep step : {
             mm6::detail::AtomicSaveStep::BeforeTempWrite,
             mm6::detail::AtomicSaveStep::AfterPartialTempWrite,
             mm6::detail::AtomicSaveStep::BeforeBackupReplace,
             mm6::detail::AtomicSaveStep::BeforePrimaryReplace}) {
        TestProfile profile;
        std::vector<std::uint8_t> old = sample(3);
        std::vector<std::uint8_t> next = sample(4);
        mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", old);
        EXPECT_THROW(mm6::detail::writeAtomicSaveFile(profile.root(), "save001.ec6", next,
            [step](mm6::detail::AtomicSaveStep reached) {
                if (reached == step)
                    throw std::runtime_error("injected MM6 write failure");
            }), std::exception);
        EXPECT_EQ(read(profile.save()), old);
        EXPECT_EQ(mm6::loadRecoverableSaveFile(profile.root(), "save001.ec6"), old);
        if (std::filesystem::exists(profile.backup()))
            EXPECT_EQ(read(profile.backup()), old);
        for (const auto &entry : std::filesystem::directory_iterator(profile.root() / "saves"))
            EXPECT_FALSE(entry.path().filename().string().contains(".tmp."));
    }
}

UNIT_TEST(MM6AtomicSaveFile, RejectsInvalidCandidateAndDestination) {
    TestProfile profile;
    std::vector<std::uint8_t> old = sample(5);
    mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", old);
    std::vector<std::uint8_t> wrongTitle = sample(6);
    wrongTitle[2] = '7';
    EXPECT_THROW(mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", wrongTitle), std::exception);
    EXPECT_THROW(mm6::writeAtomicSaveFile(profile.root(), "../save001.ec6", old), std::exception);
    EXPECT_EQ(read(profile.save()), old);

    { std::ofstream corrupt(profile.save(), std::ios::binary); corrupt << "bad"; }
    EXPECT_THROW(mm6::writeAtomicSaveFile(profile.root(), "save001.ec6", sample(7)), std::exception);
    EXPECT_EQ(read(profile.save()), std::vector<std::uint8_t>({'b', 'a', 'd'}));
    EXPECT_THROW((void) mm6::loadRecoverableSaveFile(profile.root(), "save001.ec6"), std::exception);
}
#endif
