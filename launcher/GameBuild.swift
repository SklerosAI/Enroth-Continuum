import Foundation

/** The title-locked helpers Atlas can launch without mixing campaign profiles. */
enum GameBuild: String {
    case mm6
    case fork = "mm7"
    case upstream = "openenroth"

    var helperName: String {
        switch self {
        case .mm6: return "MM6"
        case .fork: return "MM7"
        case .upstream: return "OpenEnroth"
        }
    }

    var displayName: String {
        switch self {
        case .mm6: return "MM6"
        case .fork: return "MM7"
        case .upstream: return "upstream OpenEnroth"
        }
    }

    var dataTitle: String { self == .mm6 ? "mm6" : "mm7" }

    var supportsRecording: Bool { self != .mm6 }

    func bundledHelperExecutable(in app: URL) -> URL {
        app.appendingPathComponent("Contents/Helpers/\(helperName).app/Contents/MacOS/\(helperName)")
    }

    /** Resolves an embedded helper first; upstream can instead use a single separate system installation. */
    func helperExecutable(in app: URL, externalUpstreamApp: URL? = nil) -> URL {
        let bundled = bundledHelperExecutable(in: app)
        guard self == .upstream, !FileManager.default.isExecutableFile(atPath: bundled.path) else { return bundled }
        let upstreamApp = externalUpstreamApp ?? URL(fileURLWithPath: "/Applications/OpenEnroth.app", isDirectory: true)
        return upstreamApp.appendingPathComponent("Contents/MacOS/OpenEnroth")
    }

    func isAvailable(in app: URL, externalUpstreamApp: URL? = nil) -> Bool {
        FileManager.default.isExecutableFile(atPath: helperExecutable(in: app, externalUpstreamApp: externalUpstreamApp).path)
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
        case .mm6: return supportPath.appendingPathComponent("Profiles/MM6-Atlas", isDirectory: true)
        case .fork: return MM7ProfilePath.resolve(environment: environment, supportPath: supportPath)
        case .upstream: return supportPath.appendingPathComponent("Profiles/MM7-OpenEnroth", isDirectory: true)
        }
    }

    /** Upstream OpenEnroth plays MM7 only and has no `--game` option. */
    func arguments(dataPath: URL, profilePath: URL) -> [String] {
        switch self {
        case .mm6: return ["--game", "mm6", "--data-path", dataPath.path, "--user-path", profilePath.path]
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
        case .mm6:
            environment.removeValue(forKey: "ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY")
            environment.removeValue(forKey: "OPENENROTH_MM7_PATH")
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
