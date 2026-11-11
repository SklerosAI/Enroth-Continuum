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
precondition(!GameBuild.upstream.isAvailable(in: stubApp))
let upstreamExecutable = GameBuild.upstream.helperExecutable(in: stubApp)
precondition(upstreamExecutable.path.hasSuffix("Contents/Helpers/OpenEnroth.app/Contents/MacOS/OpenEnroth"))
try fileManager.createDirectory(at: upstreamExecutable.deletingLastPathComponent(), withIntermediateDirectories: true)
try "#!/bin/sh\n".write(to: upstreamExecutable, atomically: true, encoding: .utf8)
precondition(!GameBuild.upstream.isAvailable(in: stubApp))
try fileManager.setAttributes([.posixPermissions: 0o755], ofItemAtPath: upstreamExecutable.path)
precondition(GameBuild.upstream.isAvailable(in: stubApp))

let upstreamProfile = GameBuild.upstream.profilePath(environment: [key: profilePath.path], supportPath: supportPath)
precondition(upstreamProfile == supportPath.appendingPathComponent("Profiles/MM7-OpenEnroth", isDirectory: true))
precondition(upstreamProfile != GameBuild.fork.profilePath(environment: [:], supportPath: supportPath))

let data = URL(fileURLWithPath: "/data/MM7", isDirectory: true)
let profile = URL(fileURLWithPath: "/profiles/MM7-OpenEnroth", isDirectory: true)
precondition(GameBuild.upstream.arguments(dataPath: data, profilePath: profile)
             == ["--data-path", "/data/MM7", "--user-path", "/profiles/MM7-OpenEnroth"])
precondition(GameBuild.fork.arguments(dataPath: data, profilePath: profile)
             == ["--game", "mm7", "--data-path", "/data/MM7", "--user-path", "/profiles/MM7-OpenEnroth"])

let base = ["OPENENROTH_MM7_PATH": "/elsewhere", "ENROTH_MM7_MOD_DIR": "/mods", "HOME": "/home"]
let mods = URL(fileURLWithPath: "/mods/MM7", isDirectory: true)
let upstreamEnvironment = GameBuild.upstream.environment(base: base, modRoot: mods, vendorRefresh: true)
precondition(upstreamEnvironment == ["HOME": "/home"])
let forkEnvironment = GameBuild.fork.environment(base: base, modRoot: mods, vendorRefresh: true)
precondition(forkEnvironment["ENROTH_MM7_MOD_DIR"] == "/mods/MM7")
precondition(forkEnvironment["ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY"] == "1")
precondition(GameBuild.fork.environment(base: base, modRoot: nil, vendorRefresh: true)["ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY"] == "0")

print("MM7 profile path and game build checks passed")
