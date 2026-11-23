import Foundation

let fileManager = FileManager.default
let supportPath = fileManager.temporaryDirectory.appendingPathComponent("atlas-profile-test-support", isDirectory: true)
let normalPath = supportPath.appendingPathComponent("Profiles/MM7-Atlas", isDirectory: true)
let key = MM7ProfilePath.overrideKey

precondition(MM7ProfilePath.resolve(environment: [:], supportPath: supportPath) == normalPath)
precondition(MM7ProfilePath.resolve(environment: [key: "relative/path"], supportPath: supportPath) == nil)
precondition(MM7ProfilePath.resolve(environment: [key: "/missing/mm7-visible-gate-profile"], supportPath: supportPath) == nil)

let profilePath = fileManager.temporaryDirectory.resolvingSymlinksInPath()
    .appendingPathComponent("mm7-visible-gate-test-\(UUID().uuidString)", isDirectory: true)
try fileManager.createDirectory(at: profilePath, withIntermediateDirectories: false,
                                attributes: [.posixPermissions: 0o700])
defer { try? fileManager.removeItem(at: profilePath) }
let marker = profilePath.appendingPathComponent(".ec-mm7-visible-gate")
try "EnrothContinuum-MM7-visible-gate-v1\n".write(to: marker, atomically: true, encoding: .utf8)

let selected = MM7ProfilePath.resolve(environment: [key: profilePath.path], supportPath: supportPath)
precondition(selected == profilePath)
try fileManager.removeItem(at: marker)
precondition(MM7ProfilePath.resolve(environment: [key: profilePath.path], supportPath: supportPath) == nil)
precondition(selected == profilePath) // A Play action retains its original validated URL.

try "EnrothContinuum-MM7-visible-gate-v1\n".write(to: marker, atomically: true, encoding: .utf8)
try fileManager.setAttributes([.posixPermissions: 0o755], ofItemAtPath: profilePath.path)
precondition(MM7ProfilePath.resolve(environment: [key: profilePath.path], supportPath: supportPath) == nil)

// The upstream button stays disabled until its helper is bundled, and never shares the fork's profile.
let stubApp = fileManager.temporaryDirectory.resolvingSymlinksInPath()
    .appendingPathComponent("atlas-build-test-\(UUID().uuidString).app", isDirectory: true)
defer { try? fileManager.removeItem(at: stubApp) }
let forkExecutable = GameBuild.fork.helperExecutable(in: stubApp)
try fileManager.createDirectory(at: forkExecutable.deletingLastPathComponent(), withIntermediateDirectories: true)
try "#!/bin/sh\n".write(to: forkExecutable, atomically: true, encoding: .utf8)
try fileManager.setAttributes([.posixPermissions: 0o755], ofItemAtPath: forkExecutable.path)
precondition(GameBuild.fork.isAvailable(in: stubApp))
precondition(!GameBuild.mm6.isAvailable(in: stubApp))
let externalUpstream = fileManager.temporaryDirectory.resolvingSymlinksInPath()
    .appendingPathComponent("atlas-external-upstream-\(UUID().uuidString).app", isDirectory: true)
defer { try? fileManager.removeItem(at: externalUpstream) }
precondition(!GameBuild.upstream.isAvailable(in: stubApp, externalUpstreamApp: externalUpstream))
let mm6Executable = GameBuild.mm6.helperExecutable(in: stubApp)
try fileManager.createDirectory(at: mm6Executable.deletingLastPathComponent(), withIntermediateDirectories: true)
try "#!/bin/sh\n".write(to: mm6Executable, atomically: true, encoding: .utf8)
try fileManager.setAttributes([.posixPermissions: 0o755], ofItemAtPath: mm6Executable.path)
precondition(GameBuild.mm6.isAvailable(in: stubApp))
let upstreamExecutable = GameBuild.upstream.bundledHelperExecutable(in: stubApp)
precondition(upstreamExecutable.path.hasSuffix("Contents/Helpers/OpenEnroth.app/Contents/MacOS/OpenEnroth"))
let externalExecutable = externalUpstream.appendingPathComponent("Contents/MacOS/OpenEnroth")
try fileManager.createDirectory(at: externalExecutable.deletingLastPathComponent(), withIntermediateDirectories: true)
try "#!/bin/sh\n".write(to: externalExecutable, atomically: true, encoding: .utf8)
try fileManager.setAttributes([.posixPermissions: 0o755], ofItemAtPath: externalExecutable.path)
precondition(GameBuild.upstream.helperExecutable(in: stubApp, externalUpstreamApp: externalUpstream) == externalExecutable)
precondition(GameBuild.upstream.isAvailable(in: stubApp, externalUpstreamApp: externalUpstream))
try fileManager.createDirectory(at: upstreamExecutable.deletingLastPathComponent(), withIntermediateDirectories: true)
try "#!/bin/sh\n".write(to: upstreamExecutable, atomically: true, encoding: .utf8)
precondition(!GameBuild.upstream.isAvailable(in: stubApp, externalUpstreamApp: URL(fileURLWithPath: "/missing")))
try fileManager.setAttributes([.posixPermissions: 0o755], ofItemAtPath: upstreamExecutable.path)
precondition(GameBuild.upstream.helperExecutable(in: stubApp, externalUpstreamApp: externalUpstream) == upstreamExecutable)
precondition(GameBuild.upstream.isAvailable(in: stubApp, externalUpstreamApp: externalUpstream))

let upstreamProfile = GameBuild.upstream.profilePath(environment: [key: profilePath.path], supportPath: supportPath)
precondition(upstreamProfile == supportPath.appendingPathComponent("Profiles/MM7-OpenEnroth", isDirectory: true))
precondition(upstreamProfile != GameBuild.fork.profilePath(environment: [:], supportPath: supportPath))
precondition(GameBuild.mm6.profilePath(environment: [key: profilePath.path], supportPath: supportPath)
             == supportPath.appendingPathComponent("Profiles/MM6-Atlas", isDirectory: true))

let data = URL(fileURLWithPath: "/data/MM7", isDirectory: true)
let profile = URL(fileURLWithPath: "/profiles/MM7-OpenEnroth", isDirectory: true)
precondition(GameBuild.upstream.arguments(dataPath: data, profilePath: profile)
             == ["--data-path", "/data/MM7", "--user-path", "/profiles/MM7-OpenEnroth"])
precondition(GameBuild.fork.arguments(dataPath: data, profilePath: profile)
             == ["--game", "mm7", "--data-path", "/data/MM7", "--user-path", "/profiles/MM7-OpenEnroth"])
precondition(GameBuild.mm6.arguments(dataPath: URL(fileURLWithPath: "/data/MM6"), profilePath: profile)
             == ["--game", "mm6", "--data-path", "/data/MM6", "--user-path", "/profiles/MM7-OpenEnroth"])

let base = ["OPENENROTH_MM7_PATH": "/elsewhere", "ENROTH_MM7_MOD_DIR": "/mods",
            "ENROTH_MM7_ALL_ARTIFACTS_STORAGE": "1", "ENROTH_MM7_CHEST_REROLL": "1",
            "ENROTH_MM7_MONSTER_LOOT_REROLL": "1", "HOME": "/home"]
let mods = URL(fileURLWithPath: "/mods/MM7", isDirectory: true)
let upstreamEnvironment = GameBuild.upstream.environment(base: base, modRoot: mods, vendorRefresh: true,
                                                         allArtifactsStorage: true, chestReroll: true, monsterLootReroll: true)
precondition(upstreamEnvironment == ["HOME": "/home"])
let forkEnvironment = GameBuild.fork.environment(base: base, modRoot: mods, vendorRefresh: true,
                                                 allArtifactsStorage: true, chestReroll: true, monsterLootReroll: true)
precondition(forkEnvironment["ENROTH_MM7_MOD_DIR"] == "/mods/MM7")
precondition(forkEnvironment["ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY"] == "1")
precondition(forkEnvironment["ENROTH_MM7_ALL_ARTIFACTS_STORAGE"] == "1")
precondition(forkEnvironment["ENROTH_MM7_CHEST_REROLL"] == "1")
precondition(forkEnvironment["ENROTH_MM7_MONSTER_LOOT_REROLL"] == "1")
let vanillaEnvironment = GameBuild.fork.environment(base: base, modRoot: nil, vendorRefresh: true,
                                                    allArtifactsStorage: true, chestReroll: true, monsterLootReroll: true)
precondition(vanillaEnvironment["ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY"] == "0")
precondition(vanillaEnvironment["ENROTH_MM7_ALL_ARTIFACTS_STORAGE"] == "0")
precondition(vanillaEnvironment["ENROTH_MM7_CHEST_REROLL"] == "0")
precondition(vanillaEnvironment["ENROTH_MM7_MONSTER_LOOT_REROLL"] == "0")
precondition(GameBuild.fork.environment(base: base, modRoot: mods, vendorRefresh: false,
                                        allArtifactsStorage: false, chestReroll: false, monsterLootReroll: false)["ENROTH_MM7_MONSTER_LOOT_REROLL"] == "0")
precondition(GameBuild.mm6.environment(base: base, modRoot: mods, vendorRefresh: true,
                                       allArtifactsStorage: true, chestReroll: true, monsterLootReroll: true)
             == ["HOME": "/home", "ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY": "1", "ENROTH_MM6_ALL_ARTIFACTS": "0",
                 "ENROTH_MM6_CHEST_REROLL": "0", "ENROTH_MM6_MONSTER_LOOT_REROLL": "0"])
precondition(GameBuild.mm6.environment(base: base, modRoot: nil, vendorRefresh: false,
                                       allArtifactsStorage: false, chestReroll: false, monsterLootReroll: false)
             == ["HOME": "/home", "ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY": "0", "ENROTH_MM6_ALL_ARTIFACTS": "0",
                 "ENROTH_MM6_CHEST_REROLL": "0", "ENROTH_MM6_MONSTER_LOOT_REROLL": "0"])
precondition(GameBuild.mm6.environment(base: base, modRoot: nil, vendorRefresh: false, allArtifactsStorage: false,
                                       chestReroll: false, monsterLootReroll: false, mm6AllArtifacts: true)
             == ["HOME": "/home", "ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY": "0", "ENROTH_MM6_ALL_ARTIFACTS": "1",
                 "ENROTH_MM6_CHEST_REROLL": "0", "ENROTH_MM6_MONSTER_LOOT_REROLL": "0"])
precondition(GameBuild.mm6.environment(base: base, modRoot: nil, vendorRefresh: false, allArtifactsStorage: false,
                                       chestReroll: false, monsterLootReroll: false, mm6ChestReroll: true)
             == ["HOME": "/home", "ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY": "0", "ENROTH_MM6_ALL_ARTIFACTS": "0",
                 "ENROTH_MM6_CHEST_REROLL": "1", "ENROTH_MM6_MONSTER_LOOT_REROLL": "1"])
precondition(GameBuild.mm6.environment(base: base, modRoot: nil, vendorRefresh: false, allArtifactsStorage: false,
                                       chestReroll: false, monsterLootReroll: false, mm6MonsterLootReroll: true)
             == ["HOME": "/home", "ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY": "0", "ENROTH_MM6_ALL_ARTIFACTS": "0",
                 "ENROTH_MM6_CHEST_REROLL": "1", "ENROTH_MM6_MONSTER_LOOT_REROLL": "1"])
let mm6FlagBase = base.merging(["ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY": "1", "ENROTH_MM6_ALL_ARTIFACTS": "1",
                                 "ENROTH_MM6_CHEST_REROLL": "1", "ENROTH_MM6_MONSTER_LOOT_REROLL": "1"]) { $1 }
let mm6FromFlagBase = GameBuild.mm6.environment(base: mm6FlagBase, modRoot: nil, vendorRefresh: false,
                                                allArtifactsStorage: false, chestReroll: false, monsterLootReroll: false)
precondition(mm6FromFlagBase["ENROTH_MM6_ALL_ARTIFACTS"] == "0")
precondition(mm6FromFlagBase["ENROTH_MM6_CHEST_REROLL"] == "0")
precondition(mm6FromFlagBase["ENROTH_MM6_MONSTER_LOOT_REROLL"] == "0")
for modRoot in [mods, nil] {
    let forkWithMM6Flags = GameBuild.fork.environment(base: mm6FlagBase, modRoot: modRoot, vendorRefresh: true,
                                                      allArtifactsStorage: true, chestReroll: true, monsterLootReroll: true,
                                                      mm6AllArtifacts: true, mm6ChestReroll: true, mm6MonsterLootReroll: true)
    precondition(forkWithMM6Flags["ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY"] == nil)
    precondition(forkWithMM6Flags["ENROTH_MM6_ALL_ARTIFACTS"] == nil)
    precondition(forkWithMM6Flags["ENROTH_MM6_CHEST_REROLL"] == nil)
    precondition(forkWithMM6Flags["ENROTH_MM6_MONSTER_LOOT_REROLL"] == nil)
    precondition(forkWithMM6Flags["ENROTH_MM7_CHEST_REROLL"] == (modRoot == nil ? "0" : "1"))
    precondition(forkWithMM6Flags["ENROTH_MM7_MONSTER_LOOT_REROLL"] == (modRoot == nil ? "0" : "1"))
}
precondition(GameBuild.upstream.environment(base: mm6FlagBase, modRoot: mods, vendorRefresh: true,
                                            allArtifactsStorage: true, chestReroll: true, monsterLootReroll: true,
                                            mm6AllArtifacts: true, mm6ChestReroll: true, mm6MonsterLootReroll: true)
             == ["HOME": "/home"])

// Either old MM7 reroll choice enables the complete mod; vanilla still suppresses both.
for choices in [(true, false), (false, true)] {
    let merged = GameBuild.fork.environment(base: base, modRoot: mods, vendorRefresh: false,
                                             allArtifactsStorage: false, chestReroll: choices.0,
                                             monsterLootReroll: choices.1)
    precondition(merged["ENROTH_MM7_CHEST_REROLL"] == "1")
    precondition(merged["ENROTH_MM7_MONSTER_LOOT_REROLL"] == "1")
}

print("MM7 profile path and game build checks passed")
