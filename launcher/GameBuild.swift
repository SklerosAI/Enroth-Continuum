import Foundation

/** The two engines Atlas can launch on MM7 data. */
enum GameBuild: String {
    case fork = "mm7"
    case upstream = "openenroth"

    var helperName: String { self == .fork ? "MM7" : "OpenEnroth" }

    var displayName: String { self == .fork ? "MM7" : "upstream OpenEnroth" }

    func helperExecutable(in app: URL) -> URL {
        app.appendingPathComponent("Contents/Helpers/\(helperName).app/Contents/MacOS/\(helperName)")
    }

    func isAvailable(in app: URL) -> Bool {
        FileManager.default.isExecutableFile(atPath: helperExecutable(in: app).path)
    }

    /**
     * Profile folder for this build. Upstream never shares the fork's profile, since its saves and
     * config would mix with the fork's.
     *
     * @param environment               Process environment, for the fork's visible-gate override.
     * @param supportPath               Atlas's Application Support folder.
     * @return                          The profile folder, or nil when the fork's override is invalid.
     */
    func profilePath(environment: [String: String], supportPath: URL) -> URL? {
        switch self {
        case .fork: return MM7ProfilePath.resolve(environment: environment, supportPath: supportPath)
        case .upstream: return supportPath.appendingPathComponent("Profiles/MM7-OpenEnroth", isDirectory: true)
        }
    }

    /** Upstream OpenEnroth plays MM7 only and has no `--game` option. */
    func arguments(dataPath: URL, profilePath: URL) -> [String] {
        switch self {
        case .fork: return ["--game", "mm7", "--data-path", dataPath.path, "--user-path", profilePath.path]
        case .upstream: return ["--data-path", dataPath.path, "--user-path", profilePath.path]
        }
    }

    /**
     * Child environment for the helper.
     *
     * @param base                      Atlas's own environment.
     * @param modRoot                   Private mods folder when Private mods is on, else nil.
     * @param vendorRefresh             Whether Private mods asks for fresh vendor stock.
     * @return                          The environment to launch the helper with.
     */
    func environment(base: [String: String], modRoot: URL?, vendorRefresh: Bool) -> [String: String] {
        var environment = base
        environment.removeValue(forKey: "ENROTH_MM7_MOD_DIR")
        switch self {
        case .fork:
            environment["ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY"] = "0"
            if let modRoot {
                environment["ENROTH_MM7_MOD_DIR"] = modRoot.path
                if vendorRefresh { environment["ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY"] = "1" }
            }
        case .upstream:
            environment.removeValue(forKey: "ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY")
            environment.removeValue(forKey: "OPENENROTH_MM7_PATH") // Upstream's fallback MM7 data path.
        }
        return environment
    }
}
