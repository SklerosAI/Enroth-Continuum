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

print("MM7 profile path checks passed")
