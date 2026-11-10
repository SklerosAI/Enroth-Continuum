import Darwin
import Foundation

enum MM7ProfilePath {
    static let overrideKey = "ENROTH_CONTINUUM_MM7_DISPOSABLE_PROFILE"

    static func resolve(environment: [String: String], supportPath: URL) -> URL? {
        guard let raw = environment[overrideKey] else {
            return supportPath.appendingPathComponent("Profiles/MM7-Atlas", isDirectory: true)
        }
        guard raw.hasPrefix("/") else { return nil }
        let candidate = URL(fileURLWithPath: raw, isDirectory: true).standardizedFileURL
        let resolved = candidate.resolvingSymlinksInPath()
        let temp = FileManager.default.temporaryDirectory.resolvingSymlinksInPath()
        guard candidate.path == resolved.path,
              resolved.deletingLastPathComponent().path == temp.path,
              resolved.lastPathComponent.hasPrefix("mm7-visible-gate-") else { return nil }
        guard let attributes = try? FileManager.default.attributesOfItem(atPath: resolved.path),
              attributes[.type] as? FileAttributeType == .typeDirectory,
              let owner = attributes[.ownerAccountID] as? NSNumber,
              owner.intValue == Int(getuid()),
              let permissions = attributes[.posixPermissions] as? NSNumber,
              permissions.intValue & 0o777 == 0o700 else { return nil }
        let marker = resolved.appendingPathComponent(".ec-mm7-visible-gate")
        guard (try? String(contentsOf: marker, encoding: .utf8)) == "EnrothContinuum-MM7-visible-gate-v1\n" else { return nil }
        return resolved
    }
}
