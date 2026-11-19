#include "MM6PrivateMapSource.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "MM6MapSaveValidation.h"

#include "Library/Json/Json.h"

#include "Utility/Sha256.h"

namespace mm6 {
namespace {

constexpr std::string_view GATE = "F3_GATE_REPORT_HASHES_2026-11-06.json";
constexpr std::string_view IDENTITY = "F3_IDENTITY_2026-11-05.json";
constexpr std::string_view CAPTURE = "F3_CAPTURE_2026-11-05.json";
constexpr std::string_view REPORT = "F3_CONTENT_RECONCILIATION_2026-11-05.json";
constexpr std::string_view SIGNOFF = "F3_CONTENT_SCHEMA_SIGNOFF_2026-11-05.json";
constexpr std::string_view PROBE = "map-probe.json";
constexpr std::string_view MANIFEST = "MM6_EC6_MAP_NAME_MANIFEST_F4.json";
constexpr size_t MAX_REPORT_BYTES = 4 * 1024 * 1024;
constexpr size_t MAX_BASE_BYTES = 64 * 1024 * 1024;

bool within(const std::filesystem::path &root, const std::filesystem::path &candidate) {
    auto child = candidate.begin();
    for (auto parent = root.begin(); parent != root.end(); ++parent, ++child)
        if (child == candidate.end() || *child != *parent)
            return false;
    return true;
}

std::string readContained(const std::filesystem::path &root, const std::filesystem::path &relative,
                          size_t limit) {
    if (relative.is_absolute())
        throw std::runtime_error("absolute MM6 private map member path");
    const std::filesystem::path path = root / relative;
    if (!std::filesystem::is_regular_file(std::filesystem::symlink_status(path)))
        throw std::runtime_error("MM6 private map member is not a regular file");
    const std::filesystem::path resolved = std::filesystem::canonical(path);
    if (!within(root, resolved))
        throw std::runtime_error("MM6 private map member escapes its root");
    const auto size = std::filesystem::file_size(resolved);
    if (size > limit)
        throw std::runtime_error("MM6 private map member exceeds size limit");
    std::ifstream stream(resolved, std::ios::binary);
    if (!stream)
        throw std::runtime_error("cannot open MM6 private map member");
    std::string bytes(static_cast<size_t>(size), '\0');
    if (size && !stream.read(bytes.data(), bytes.size()))
        throw std::runtime_error("cannot read MM6 private map member");
    if (stream.peek() != std::char_traits<char>::eof())
        throw std::runtime_error("MM6 private map member changed during read");
    return bytes;
}

std::string hashString(const Json &value) {
    if (!value.is_string())
        throw std::runtime_error("invalid MM6 private map hash");
    const std::string result = value.get<std::string>();
    if (result.size() != 64 || !std::all_of(result.begin(), result.end(), [](char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        }))
        throw std::runtime_error("invalid MM6 private map hash");
    return result;
}

bool validName(std::string_view name, bool delta) {
    if (name.size() < 5 || name.size() > 16)
        return false;
    const bool suffix = delta ? (name.ends_with(".dlv") || name.ends_with(".ddm")) :
                                (name.ends_with(".blv") || name.ends_with(".odm"));
    return suffix && std::all_of(name.begin(), name.end() - 4, [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
    });
}

std::string baseName(std::string_view deltaName) {
    return std::string(deltaName.substr(0, deltaName.size() - 4)) +
           (deltaName.ends_with(".dlv") ? ".blv" : ".odm");
}

void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

} // namespace

PrivateMapSource::PrivateMapSource(const std::filesystem::path &researchRoot) {
    require(researchRoot.is_absolute() &&
            std::filesystem::is_directory(std::filesystem::symlink_status(researchRoot)),
            "MM6 private map root must be an absolute native directory");
    _root = std::filesystem::canonical(researchRoot);

    const std::string gateBytes = readContained(_root, GATE, MAX_REPORT_BYTES);
    const std::string identityBytes = readContained(_root, IDENTITY, MAX_REPORT_BYTES);
    const std::string captureBytes = readContained(_root, CAPTURE, MAX_REPORT_BYTES);
    const std::string reportBytes = readContained(_root, REPORT, MAX_REPORT_BYTES);
    const std::string signoffBytes = readContained(_root, SIGNOFF, MAX_REPORT_BYTES);
    const std::string probeBytes = readContained(_root, PROBE, MAX_REPORT_BYTES);
    const std::filesystem::path manifestPath = _root / MANIFEST;
    const auto permissions = std::filesystem::status(manifestPath).permissions();
    constexpr auto GROUP_OTHER = std::filesystem::perms::group_all | std::filesystem::perms::others_all;
    require((permissions & GROUP_OTHER) == std::filesystem::perms::none,
            "MM6 private map manifest must be owner-only");
    const std::string manifestBytes = readContained(_root, MANIFEST, 64 * 1024);

    const Json gate = Json::parse(gateBytes);
    const Json identity = Json::parse(identityBytes);
    const Json capture = Json::parse(captureBytes);
    const Json report = Json::parse(reportBytes);
    const Json signoff = Json::parse(signoffBytes);
    const Json probe = Json::parse(probeBytes);
    const Json manifest = Json::parse(manifestBytes);
    require(hashString(gate.at("reports_sha256").at(std::string(IDENTITY))) == sha256(identityBytes) &&
            hashString(gate.at("reports_sha256").at(std::string(CAPTURE))) == sha256(captureBytes) &&
            hashString(gate.at("reports_sha256").at(std::string(REPORT))) == sha256(reportBytes) &&
            hashString(gate.at("reports_sha256").at(std::string(SIGNOFF))) == sha256(signoffBytes),
            "MM6 private map F3 gate hash mismatch");
    require(hashString(report.at("capture_sha256")) == sha256(captureBytes) &&
            hashString(signoff.at("f3_content_report_sha256")) == sha256(reportBytes) &&
            hashString(capture.at("retained_map_probe_sha256")) == sha256(probeBytes),
            "MM6 private map F3 report chain mismatch");
    require(capture.at("status") == "pass" && report.at("status") == "pass" &&
            signoff.at("status") == "pass" &&
            identity.at("summary").at("match") == 23 &&
            identity.at("summary").at("mismatch") == 0 &&
            identity.at("summary").at("missing_or_symlink") == 0,
            "MM6 private map F3 source gate is incomplete");
    require(manifest.is_object() && manifest.size() == 5 &&
            manifest.at("format") == "mm6-ec6-map-name-manifest-v1" &&
            hashString(manifest.at("source_gate_sha256")) == sha256(gateBytes) &&
            hashString(manifest.at("source_probe_sha256")) == sha256(probeBytes),
            "MM6 private map manifest source identity mismatch");

    const Json &signedMaps = signoff.at("checks").at("maps");
    require(signedMaps.at("count") == 134 && signedMaps.at("matches") == 134 &&
            signedMaps.at("rows").is_array() && signedMaps.at("rows").size() == 134,
            "MM6 private map F3 signoff is incomplete");
    const Json &probeMaps = probe.at("maps");
    require(probeMaps.is_array() && probeMaps.size() == 134 &&
            probe.at("summary").at("entries") == 134 &&
            probe.at("missing_pairs").empty() && probe.at("extra_pairs").empty() &&
            report.at("families").at("map_pairs").at("count") == 134 &&
            report.at("families").at("map_pairs").at("status_counts").at("match") == 134 &&
            report.at("probe_checks").at(std::string(PROBE)).at("sha256_matches") == 134 &&
            report.at("probe_checks").at(std::string(PROBE)).at("mismatches").empty(),
            "MM6 private map F3 probe is incomplete");
    std::unordered_map<std::string, std::pair<std::string, std::string>> probeHashes;
    for (const Json &row : probeMaps) {
        const std::string name = row.at("name").get<std::string>();
        require(validName(name, true) || validName(name, false), "invalid MM6 F3 map name");
        require(probeHashes.emplace(name, std::make_pair(hashString(row.at("sha256")),
                                                   hashString(row.at("decoded_sha256")))).second,
                "duplicate MM6 F3 map name");
    }
    for (const Json &row : signedMaps.at("rows")) {
        const std::string name = row.at("name").get<std::string>();
        auto it = probeHashes.find(name);
        require(row.at("match") == true && it != probeHashes.end() &&
                hashString(row.at("source_sha256")) == it->second.first,
                "MM6 private map F3 signoff row mismatch");
    }

    const Json &entries = manifest.at("entries");
    require(entries.is_array() && entries.size() == 67, "MM6 private map manifest requires 67 entries");
    std::string joinedNames;
    size_t indoorCount = 0;
    size_t outdoorCount = 0;
    for (const Json &entry : entries) {
        require(entry.is_object() && entry.size() == 2, "invalid MM6 private map manifest entry");
        const std::string name = entry.at("name").get<std::string>();
        require(validName(name, true) && (_expectedNames.empty() || _expectedNames.back() < name),
                "MM6 private map manifest names are invalid or unsorted");
        const std::string base = baseName(name);
        auto deltaIt = probeHashes.find(name);
        auto baseIt = probeHashes.find(base);
        require(deltaIt != probeHashes.end() && baseIt != probeHashes.end() &&
                hashString(entry.at("decoded_sha256")) == deltaIt->second.second,
                "MM6 private map manifest differs from F3 probe");
        _expectedNames.push_back(name);
        _baseHashes.emplace(base, baseIt->second.second);
        joinedNames += name + '\n';
        name.ends_with(".dlv") ? ++indoorCount : ++outdoorCount;
    }
    require(indoorCount == 52 && outdoorCount == 15 && _baseHashes.size() == 67 &&
            hashString(manifest.at("names_sha256")) == sha256(joinedNames),
            "MM6 private map manifest set is incomplete");
}

Blob PrivateMapSource::readBase(std::string_view baseName) const {
    const auto it = _baseHashes.find(std::string(baseName));
    if (it == _baseHashes.end())
        throw std::runtime_error("MM6 private map base name is not in the F3 set");
    const std::filesystem::path directory = baseName.ends_with(".blv") ? "F4_INDOOR_DECODED" :
                                                                    "F4_OUTDOOR_DECODED";
    const std::string bytes = readContained(_root, directory / baseName, MAX_BASE_BYTES);
    require(sha256(bytes) == it->second, "MM6 private map base hash differs from F3");
    return Blob::fromString(bytes);
}

void loadKnownSaveWithPrivateMapsBeforeMutation(std::span<const std::uint8_t> bytes,
                                                const std::filesystem::path &researchRoot,
                                                const GroundItemVerifier &verifyGroundItem,
                                                KnownSaveFields *destination) {
    if (!destination)
        throw std::runtime_error("null MM6 private map save destination");
    const PrivateMapSource source(researchRoot);
    loadKnownSaveWithMapBasesBeforeMutation(bytes, source.expectedNames(),
        [&source](std::string_view name) { return source.readBase(name); }, verifyGroundItem, destination);
}

} // namespace mm6
