#include <array>
#include <cstdint>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "Testing/Unit/UnitTest.h"

#include "Engine/MM7/MM7AtomicSaveFile.h"

namespace {

#ifndef _WIN32
class TestProfile {
public:
  TestProfile() {
    std::string pattern =
        (std::filesystem::temp_directory_path() / "mm7-atomic-XXXXXX").string();
    char *result = ::mkdtemp(pattern.data());
    if (!result)
      throw std::runtime_error("cannot create MM7 test profile");
    _root = result;
  }

  ~TestProfile() {
    std::error_code error;
    std::filesystem::remove_all(_root, error);
  }

  const std::filesystem::path &root() const { return _root; }
  std::filesystem::path primary() const { return _root / "saves/save001.mm7"; }
  std::filesystem::path backup() const {
    return _root / "saves/save001.mm7.bak";
  }

private:
  std::filesystem::path _root;
};

std::vector<std::uint8_t> sample(std::uint8_t marker) {
  return {'M', marker, static_cast<std::uint8_t>('M' ^ marker)};
}

bool validate(std::span<const std::uint8_t> bytes) {
  return bytes.size() == 3 && bytes[0] == 'M' &&
         bytes[2] == static_cast<std::uint8_t>(bytes[0] ^ bytes[1]);
}

std::vector<std::uint8_t> read(const std::filesystem::path &path) {
  std::ifstream input(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}

void write(const std::filesystem::path &path,
           const std::vector<std::uint8_t> &bytes) {
  std::ofstream output(path, std::ios::binary);
  output.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

size_t tempCount(const std::filesystem::path &directory) {
  size_t count = 0;
  for (const auto &entry : std::filesystem::directory_iterator(directory))
    if (entry.path().filename().string().starts_with(".save001.mm7.tmp."))
      ++count;
  return count;
}
#endif

} // namespace

#ifndef _WIN32
UNIT_TEST(MM7AtomicSaveFile, NewAndOverwrittenSlotPreserveOnePriorPrimary) {
  TestProfile profile;
  const auto first = sample(1);
  const auto second = sample(2);
  const auto third = sample(3);
  EXPECT_FALSE(mm7::preflightSaveSlot(profile.root(), "save001.mm7", validate));

  mm7::writeAtomicSaveSlot(profile.root(), "save001.mm7", first, validate);
  EXPECT_EQ(read(profile.primary()), first);
  EXPECT_FALSE(std::filesystem::exists(profile.backup()));
  mm7::writeAtomicSaveSlot(profile.root(), "save001.mm7", second, validate);
  EXPECT_EQ(read(profile.primary()), second);
  EXPECT_EQ(read(profile.backup()), first);
  mm7::writeAtomicSaveSlot(profile.root(), "save001.mm7", third, validate);
  EXPECT_EQ(read(profile.primary()), third);
  EXPECT_EQ(read(profile.backup()), second);
  const auto chosen =
      mm7::preflightSaveSlot(profile.root(), "save001.mm7", validate);
  EXPECT_TRUE(chosen);
  EXPECT_FALSE(chosen->fromBackup);
  EXPECT_EQ(chosen->bytes, third);
  EXPECT_EQ(tempCount(profile.root() / "saves"), 0);
}

UNIT_TEST(MM7AtomicSaveFile, FaultsBeforePrimaryReplaceKeepPriorPrimary) {
  const std::array steps = {
      mm7::detail::AtomicSaveStep::BeforeTempWrite,
      mm7::detail::AtomicSaveStep::AfterPartialTempWrite,
      mm7::detail::AtomicSaveStep::BeforeBackupReplace,
      mm7::detail::AtomicSaveStep::BeforePrimaryReplace,
  };
  for (const auto step : steps) {
    TestProfile profile;
    const auto first = sample(10);
    const auto second = sample(11);
    const auto third = sample(12);
    mm7::writeAtomicSaveSlot(profile.root(), "save001.mm7", first, validate);
    mm7::writeAtomicSaveSlot(profile.root(), "save001.mm7", second, validate);
    const auto failAtStep = [step](mm7::detail::AtomicSaveStep current) {
      if (current == step)
        throw std::runtime_error("injected MM7 save fault");
    };
    EXPECT_THROW(mm7::detail::writeAtomicSaveSlot(profile.root(), "save001.mm7",
                                                  third, validate, failAtStep),
                 std::runtime_error);
    EXPECT_EQ(read(profile.primary()), second);
    EXPECT_EQ(read(profile.backup()),
              step == mm7::detail::AtomicSaveStep::BeforePrimaryReplace
                  ? second
                  : first);
    EXPECT_EQ(tempCount(profile.root() / "saves"), 0);
  }
}

UNIT_TEST(MM7AtomicSaveFile, PostPrimaryRenameFaultDoesNotClaimRollback) {
  TestProfile profile;
  const auto old = sample(20);
  const auto next = sample(21);
  mm7::writeAtomicSaveSlot(profile.root(), "save001.mm7", old, validate);
  const auto failAfterRename = [](mm7::detail::AtomicSaveStep step) {
    if (step ==
        mm7::detail::AtomicSaveStep::AfterPrimaryReplaceBeforeDirectorySync)
      throw std::runtime_error("injected post-rename fault");
  };
  EXPECT_THROW(mm7::detail::writeAtomicSaveSlot(profile.root(), "save001.mm7",
                                                next, validate,
                                                failAfterRename),
               std::runtime_error);
  EXPECT_EQ(read(profile.primary()), next);
  EXPECT_EQ(read(profile.backup()), old);
  const auto chosen =
      mm7::preflightSaveSlot(profile.root(), "save001.mm7", validate);
  EXPECT_TRUE(chosen);
  EXPECT_FALSE(chosen->fromBackup);
  EXPECT_EQ(chosen->bytes, next);
  EXPECT_EQ(tempCount(profile.root() / "saves"), 0);
}

UNIT_TEST(MM7AtomicSaveFile, CorruptPrimarySelectsBackupWithoutRepair) {
  TestProfile profile;
  const auto old = sample(30);
  const auto next = sample(31);
  mm7::writeAtomicSaveSlot(profile.root(), "save001.mm7", old, validate);
  mm7::writeAtomicSaveSlot(profile.root(), "save001.mm7", next, validate);
  const std::vector<std::uint8_t> corrupt = {'M', 31,
                                             0}; // Header survives; body fails.
  write(profile.primary(), corrupt);

  const auto chosen =
      mm7::preflightSaveSlot(profile.root(), "save001.mm7", validate);
  EXPECT_TRUE(chosen);
  EXPECT_TRUE(chosen->fromBackup);
  EXPECT_EQ(chosen->bytes, old);
  EXPECT_EQ(read(profile.primary()), corrupt);
  EXPECT_EQ(read(profile.backup()), old);
  EXPECT_THROW(mm7::writeAtomicSaveSlot(profile.root(), "save001.mm7",
                                        sample(32), validate),
               std::runtime_error);
  EXPECT_EQ(read(profile.primary()), corrupt);
  EXPECT_EQ(read(profile.backup()), old);
}

UNIT_TEST(MM7AtomicSaveFile, CorruptBackupAndNewSlotFaultDoNotReplaceFiles) {
  TestProfile profile;
  const auto old = sample(35);
  const auto next = sample(36);
  mm7::writeAtomicSaveSlot(profile.root(), "save001.mm7", old, validate);
  const std::vector<std::uint8_t> corrupt = {'M', 35, 0};
  write(profile.backup(), corrupt);
  EXPECT_THROW(
      mm7::writeAtomicSaveSlot(profile.root(), "save001.mm7", next, validate),
      std::runtime_error);
  EXPECT_EQ(read(profile.primary()), old);
  EXPECT_EQ(read(profile.backup()), corrupt);

  const auto failPartial = [](mm7::detail::AtomicSaveStep step) {
    if (step == mm7::detail::AtomicSaveStep::AfterPartialTempWrite)
      throw std::runtime_error("injected new-slot fault");
  };
  EXPECT_THROW(mm7::detail::writeAtomicSaveSlot(profile.root(), "save002.mm7",
                                                next, validate, failPartial),
               std::runtime_error);
  EXPECT_FALSE(std::filesystem::exists(profile.root() / "saves/save002.mm7"));
  EXPECT_FALSE(
      std::filesystem::exists(profile.root() / "saves/save002.mm7.bak"));
}

UNIT_TEST(MM7AtomicSaveFile, BackupOnlyAndDoubleCorruptionFailClosed) {
  TestProfile profile;
  std::filesystem::create_directory(profile.root() / "saves");
  const auto old = sample(40);
  write(profile.backup(), old);
  const auto chosen =
      mm7::preflightSaveSlot(profile.root(), "save001.mm7", validate);
  EXPECT_TRUE(chosen);
  EXPECT_TRUE(chosen->fromBackup);
  EXPECT_EQ(chosen->bytes, old);
  EXPECT_THROW(mm7::writeAtomicSaveSlot(profile.root(), "save001.mm7",
                                        sample(41), validate),
               std::runtime_error);
  EXPECT_FALSE(std::filesystem::exists(profile.primary()));
  EXPECT_EQ(read(profile.backup()), old);

  const std::vector<std::uint8_t> corrupt = {'M', 42, 0};
  write(profile.primary(), corrupt);
  write(profile.backup(), corrupt);
  EXPECT_THROW(
      (void)mm7::preflightSaveSlot(profile.root(), "save001.mm7", validate),
      std::runtime_error);
  EXPECT_THROW(mm7::writeAtomicSaveSlot(profile.root(), "save001.mm7",
                                        sample(43), validate),
               std::runtime_error);
  EXPECT_EQ(read(profile.primary()), corrupt);
  EXPECT_EQ(read(profile.backup()), corrupt);
}

UNIT_TEST(MM7AtomicSaveFile,
          RejectsInvalidCandidateAndUnsafePathsWithoutWriting) {
  TestProfile profile;
  const auto good = sample(50);
  const std::vector<std::uint8_t> corrupt = {'M', 50, 0};
  EXPECT_THROW(mm7::writeAtomicSaveSlot(profile.root(), "save001.mm7", corrupt,
                                        validate),
               std::invalid_argument);
  EXPECT_THROW(mm7::writeAtomicSaveSlot(profile.root(), "../save001.mm7", good,
                                        validate),
               std::invalid_argument);
  EXPECT_THROW(
      mm7::writeAtomicSaveSlot(profile.root(), "save001.ec6", good, validate),
      std::invalid_argument);
  EXPECT_THROW(
      mm7::writeAtomicSaveSlot(profile.root(), "save001.mm7", good, {}),
      std::invalid_argument);
  EXPECT_FALSE(std::filesystem::exists(profile.root() / "saves"));

  const auto other = profile.root() / "other";
  std::filesystem::create_directory(other);
  std::filesystem::create_directory_symlink(other, profile.root() / "saves");
  EXPECT_THROW(
      mm7::writeAtomicSaveSlot(profile.root(), "save001.mm7", good, validate),
      std::runtime_error);
  EXPECT_THROW(
      (void)mm7::preflightSaveSlot(profile.root(), "save001.mm7", validate),
      std::runtime_error);
  EXPECT_FALSE(std::filesystem::exists(other / "save001.mm7"));
}
#endif
