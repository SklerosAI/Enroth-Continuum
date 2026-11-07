#include "MM6V5ActorRuntimeFields.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

#include "Engine/Snapshots/EntitySnapshots.h"

#include "MM6ActiveActorSection.h"
#include "MM6SpawnedActorSection.h"

namespace mm6 {
namespace {

constexpr std::array<char, 4> OUTDOOR_TAG = {'A', 'R', 'T', 'R'};
constexpr std::array<char, 4> INDOOR_TAG = {'I', 'A', 'R', 'T'};
constexpr std::uint8_t VERSION = 1;
constexpr std::size_t MAX_ACTORS = 8192;
constexpr std::size_t IMAGE_BYTES = sizeof(Actor_MM7);
constexpr std::size_t RECORD_BYTES = IMAGE_BYTES + 4 * 3 * 4 + 8 * 3 * 4 +
    3 * 8 + 4 + 4 + 8 + 8;
constexpr std::size_t MAX_NAME_BYTES = 255;

void requireMM6(GameTitle title) {
    if (title != GameTitle::MM6)
        throw std::runtime_error("MM6 ARTR requires MM6 title");
}

void requireMapName(std::string_view name, ActorRuntimeSectionKind kind) {
    const std::string_view suffix = kind == ActorRuntimeSectionKind::OutdoorV5 ?
        ".odm" : ".blv";
    if (name.empty() || name.size() > 63 || !name.ends_with(suffix) ||
        !std::all_of(name.begin(), name.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                c == '_' || c == '.';
        }))
        throw std::runtime_error("MM6 ARTR map name is not canonical outdoor identity");
}

void put32(std::vector<std::uint8_t> *out, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        out->push_back(value >> shift);
}

void put64(std::vector<std::uint8_t> *out, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8)
        out->push_back(value >> shift);
}

class Reader {
 public:
    explicit Reader(std::span<const std::uint8_t> bytes) : _bytes(bytes) {}

    std::uint8_t u8() {
        if (_offset == _bytes.size())
            throw std::runtime_error("truncated MM6 ARTR section");
        return _bytes[_offset++];
    }

    std::uint32_t u32() {
        std::uint32_t result = 0;
        for (int shift = 0; shift < 32; shift += 8)
            result |= std::uint32_t(u8()) << shift;
        return result;
    }

    std::uint64_t u64() {
        std::uint64_t result = 0;
        for (int shift = 0; shift < 64; shift += 8)
            result |= std::uint64_t(u8()) << shift;
        return result;
    }

    std::span<const std::uint8_t> take(std::size_t size) {
        if (size > _bytes.size() - _offset)
            throw std::runtime_error("truncated MM6 ARTR record");
        const auto result = _bytes.subspan(_offset, size);
        _offset += size;
        return result;
    }

    void finish() const {
        if (_offset != _bytes.size())
            throw std::runtime_error("MM6 ARTR has trailing bytes");
    }

 private:
    std::span<const std::uint8_t> _bytes;
    std::size_t _offset = 0;
};

void requireSafeVec(const Vec3f &value) {
    if (!std::isfinite(value.x) || !std::isfinite(value.y) ||
        !std::isfinite(value.z) ||
        std::abs(value.x) > 32767 || std::abs(value.y) > 32767 ||
        std::abs(value.z) > 32767)
        throw std::runtime_error("MM6 ARTR float cannot safely reach native image");
}

bool sameFloat(float lhs, float rhs) {
    return std::bit_cast<std::uint32_t>(lhs) == std::bit_cast<std::uint32_t>(rhs);
}

bool sameVec(const Vec3f &lhs, const Vec3f &rhs) {
    return sameFloat(lhs.x, rhs.x) && sameFloat(lhs.y, rhs.y) &&
           sameFloat(lhs.z, rhs.z);
}

void putVec(std::vector<std::uint8_t> *out, const Vec3f &value) {
    requireSafeVec(value);
    put32(out, std::bit_cast<std::uint32_t>(value.x));
    put32(out, std::bit_cast<std::uint32_t>(value.y));
    put32(out, std::bit_cast<std::uint32_t>(value.z));
}

Vec3f getVec(Reader *input) {
    const float x = std::bit_cast<float>(input->u32());
    const float y = std::bit_cast<float>(input->u32());
    const float z = std::bit_cast<float>(input->u32());
    const Vec3f result(x, y, z);
    requireSafeVec(result);
    return result;
}

void putRecord(std::vector<std::uint8_t> *out, const Actor &actor,
               std::size_t slot, GameTitle title, bool indoor) {
    requireSafeVec(actor.pos);
    requireSafeVec(actor.velocity);
    requireSafeVec(actor.initialPosition);
    requireSafeVec(actor.guardingPosition);
    for (const ActorJob &job : actor.scheduledJobs)
        requireSafeVec(job.vPos);
    if (indoor && (actor.monsterInfo.name.size() > MAX_NAME_BYTES ||
                   actor.monsterInfo.internalName.size() > MAX_NAME_BYTES))
        throw std::runtime_error("MM6 IART actor runtime name exceeds bound");
    Actor imageSource = actor;
    if (indoor) {
        imageSource.monsterInfo.name.clear();
        imageSource.monsterInfo.internalName.clear();
    }
    const Actor_MM7 image = snapshotActorRuntimeImageForV5(imageSource, slot, title);
    Actor replay(static_cast<int>(slot));
    reconstruct(image, &replay);
    replay.monsterInfo.mm6Resistances = actor.monsterInfo.mm6Resistances;
    replay.donebloodsplat = actor.donebloodsplat;
    replay.massDistortionTime = actor.massDistortionTime;
    replay.pos = actor.pos;
    replay.velocity = actor.velocity;
    replay.initialPosition = actor.initialPosition;
    replay.guardingPosition = actor.guardingPosition;
    for (std::size_t index = 0; index < actor.scheduledJobs.size(); ++index)
        replay.scheduledJobs[index].vPos = actor.scheduledJobs[index].vPos;
    replay.currentActionLength = actor.currentActionLength;
    replay.currentActionTime = actor.currentActionTime;
    replay.monsterInfo.recoveryTime = actor.monsterInfo.recoveryTime;
    replay.sectorId = actor.sectorId;
    replay.hostilityGroup = actor.hostilityGroup;
    if (indoor) {
        replay.monsterInfo.name = actor.monsterInfo.name;
        replay.monsterInfo.internalName = actor.monsterInfo.internalName;
    }
    Actor repeatSource = replay;
    if (indoor) {
        repeatSource.monsterInfo.name.clear();
        repeatSource.monsterInfo.internalName.clear();
    }
    const Actor_MM7 repeated = snapshotActorRuntimeImageForV5(repeatSource, slot, title);
    if (std::memcmp(&image, &repeated, IMAGE_BYTES) != 0)
        throw std::runtime_error("MM6 ARTR actor image does not replay exactly");
    const auto *start = reinterpret_cast<const std::uint8_t *>(&image);
    out->insert(out->end(), start, start + IMAGE_BYTES);
    putVec(out, actor.pos);
    putVec(out, actor.velocity);
    putVec(out, actor.initialPosition);
    putVec(out, actor.guardingPosition);
    for (const ActorJob &job : actor.scheduledJobs)
        putVec(out, job.vPos);
    put64(out, std::bit_cast<std::uint64_t>(actor.currentActionLength.ticks()));
    put64(out, std::bit_cast<std::uint64_t>(actor.currentActionTime.ticks()));
    put64(out, std::bit_cast<std::uint64_t>(actor.monsterInfo.recoveryTime.ticks()));
    put32(out, std::bit_cast<std::uint32_t>(actor.sectorId));
    put32(out, std::bit_cast<std::uint32_t>(std::to_underlying(actor.hostilityGroup)));
    const MonsterResistances &res = *actor.monsterInfo.mm6Resistances;
    out->insert(out->end(), {res.fire, res.electricity, res.cold,
                            res.poison, res.physical, res.magic});
    out->push_back(actor.donebloodsplat ? 1 : 0);
    out->push_back(0);
    put64(out, std::bit_cast<std::uint64_t>(actor.massDistortionTime.ticks()));
    if (indoor) {
        out->push_back(actor.monsterInfo.name.size());
        out->insert(out->end(), actor.monsterInfo.name.begin(), actor.monsterInfo.name.end());
        out->push_back(actor.monsterInfo.internalName.size());
        out->insert(out->end(), actor.monsterInfo.internalName.begin(),
                    actor.monsterInfo.internalName.end());
    }
}

Actor getRecord(Reader *input, std::size_t slot, bool indoor) {
    Actor_MM7 image{};
    const auto bytes = input->take(IMAGE_BYTES);
    std::memcpy(&image, bytes.data(), bytes.size());
    Actor result(static_cast<int>(slot));
    reconstruct(image, &result);
    result.pos = getVec(input);
    result.velocity = getVec(input);
    result.initialPosition = getVec(input);
    result.guardingPosition = getVec(input);
    for (ActorJob &job : result.scheduledJobs)
        job.vPos = getVec(input);
    result.currentActionLength = Duration::fromTicks(
        std::bit_cast<std::int64_t>(input->u64()));
    result.currentActionTime = Duration::fromTicks(
        std::bit_cast<std::int64_t>(input->u64()));
    result.monsterInfo.recoveryTime = Duration::fromTicks(
        std::bit_cast<std::int64_t>(input->u64()));
    result.sectorId = std::bit_cast<std::int32_t>(input->u32());
    result.hostilityGroup = static_cast<MonsterType>(
        std::bit_cast<std::int32_t>(input->u32()));
    MonsterResistances res;
    res.fire = input->u8();
    res.electricity = input->u8();
    res.cold = input->u8();
    res.poison = input->u8();
    res.physical = input->u8();
    res.magic = input->u8();
    result.monsterInfo.mm6Resistances = res;
    const std::uint8_t blood = input->u8();
    if (blood > 1 || input->u8() != 0)
        throw std::runtime_error("MM6 ARTR blood or padding is noncanonical");
    result.donebloodsplat = blood;
    result.massDistortionTime = Duration::fromTicks(
        std::bit_cast<std::int64_t>(input->u64()));
    if (indoor) {
        const auto display = input->take(input->u8());
        result.monsterInfo.name.assign(display.begin(), display.end());
        const auto internal = input->take(input->u8());
        result.monsterInfo.internalName.assign(internal.begin(), internal.end());
    }
    return result;
}

} // namespace

ActorRuntimeSaveFields captureActorRuntimeFields(
    std::string_view mapName, std::span<const std::uint8_t> baselineActr,
    std::span<const Actor> live, GameTitle title) {
    requireMM6(title);
    ActorRuntimeSaveFields result;
    result.mapName = mapName;
    result.actors.assign(live.begin(), live.end());
    requireActorRuntimeBaselineAgreement(result, mapName, baselineActr, title);
    (void) encodeActorRuntimeSection(result, title);
    return result;
}

SaveSection encodeActorRuntimeSection(const ActorRuntimeSaveFields &fields,
                                      GameTitle title) {
    return encodeActorRuntimeSectionAs(fields, ActorRuntimeSectionKind::OutdoorV5, title);
}

SaveSection encodeActorRuntimeSectionAs(const ActorRuntimeSaveFields &fields,
                                        ActorRuntimeSectionKind kind,
                                        GameTitle title) {
    requireMM6(title);
    requireMapName(fields.mapName, kind);
    if (fields.actors.size() > MAX_ACTORS ||
        (kind == ActorRuntimeSectionKind::OutdoorV5 && fields.nativePrefixCount) ||
        (kind == ActorRuntimeSectionKind::IndoorV6 &&
         fields.nativePrefixCount > fields.actors.size()))
        throw std::runtime_error("MM6 ARTR exceeds bounded actor count");
    std::vector<std::uint8_t> bytes;
    const std::size_t extra = kind == ActorRuntimeSectionKind::IndoorV6 ? 4 : 0;
    bytes.reserve(2 + fields.mapName.size() + 4 + extra +
                  fields.actors.size() * (RECORD_BYTES +
                  (kind == ActorRuntimeSectionKind::IndoorV6 ? 2 : 0)));
    bytes.push_back(VERSION);
    bytes.push_back(fields.mapName.size());
    bytes.insert(bytes.end(), fields.mapName.begin(), fields.mapName.end());
    put32(&bytes, fields.actors.size());
    if (kind == ActorRuntimeSectionKind::IndoorV6)
        put32(&bytes, fields.nativePrefixCount);
    for (std::size_t index = 0; index < fields.actors.size(); ++index)
        putRecord(&bytes, fields.actors[index], index, title,
                  kind == ActorRuntimeSectionKind::IndoorV6);
    return {kind == ActorRuntimeSectionKind::OutdoorV5 ? OUTDOOR_TAG : INDOOR_TAG,
            true, std::move(bytes)};
}

ActorRuntimeSaveFields decodeActorRuntimeSection(const SaveContainer &container,
                                                 GameTitle title) {
    return decodeActorRuntimeSectionAs(container, ActorRuntimeSectionKind::OutdoorV5, title);
}

ActorRuntimeSaveFields decodeActorRuntimeSectionAs(
    const SaveContainer &container, ActorRuntimeSectionKind kind,
    GameTitle title) {
    requireMM6(title);
    const auto &tag = kind == ActorRuntimeSectionKind::OutdoorV5 ?
        OUTDOOR_TAG : INDOOR_TAG;
    const SaveSection *section = nullptr;
    for (const SaveSection &candidate : container.sections)
        if (candidate.tag == tag) {
            if (section)
                throw std::runtime_error("duplicate MM6 ARTR section");
            section = &candidate;
        }
    if (!section || !section->required || section->payload.size() < 6 ||
        section->payload.size() >
            2 + 63 + 8 + MAX_ACTORS * (RECORD_BYTES + 2 + 2 * MAX_NAME_BYTES))
        throw std::runtime_error("missing or unbounded MM6 ARTR section");
    Reader input(section->payload);
    if (input.u8() != VERSION)
        throw std::runtime_error("unsupported MM6 ARTR version");
    const auto name = input.take(input.u8());
    ActorRuntimeSaveFields result;
    result.mapName.assign(name.begin(), name.end());
    requireMapName(result.mapName, kind);
    const std::uint32_t count = input.u32();
    if (kind == ActorRuntimeSectionKind::IndoorV6)
        result.nativePrefixCount = input.u32();
    const std::size_t extra = kind == ActorRuntimeSectionKind::IndoorV6 ? 4 : 0;
    if (count > MAX_ACTORS || result.nativePrefixCount > count ||
        (kind == ActorRuntimeSectionKind::OutdoorV5 &&
         section->payload.size() != 2 + name.size() + 4 + count * RECORD_BYTES) ||
        (kind == ActorRuntimeSectionKind::IndoorV6 &&
         section->payload.size() < 2 + name.size() + 4 + extra +
                                   count * (RECORD_BYTES + 2)))
        throw std::runtime_error("MM6 ARTR count differs from payload");
    result.actors.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index)
        result.actors.push_back(getRecord(
            &input, index, kind == ActorRuntimeSectionKind::IndoorV6));
    input.finish();
    if (encodeActorRuntimeSectionAs(result, kind, title) != *section)
        throw std::runtime_error("MM6 ARTR is not canonical");
    return result;
}

void requireActorRuntimeBaselineAgreement(
    const ActorRuntimeSaveFields &fields, std::string_view mapName,
    std::span<const std::uint8_t> baselineActr, GameTitle title) {
    requireMM6(title);
    if (fields.mapName != mapName)
        throw std::runtime_error("MM6 ARTR map differs from HEAD");
    const auto baseline = decodeActiveActorSection(baselineActr, title);
    if (fields.actors.size() != baseline.size())
        throw std::runtime_error("MM6 ARTR count differs from ACTR baseline");
    for (std::size_t index = 0; index < baseline.size(); ++index) {
        const Actor &old = baseline[index];
        const Actor &current = fields.actors[index];
        if (current.id != index || old.id != index ||
            current.npcId != old.npcId ||
            current.monsterInfo.id != old.monsterInfo.id ||
            current.monsterId != old.monsterId ||
            !sameVec(current.initialPosition, old.initialPosition))
            throw std::runtime_error("MM6 ARTR actor PID or monster anchor changed");
    }
}

} // namespace mm6
