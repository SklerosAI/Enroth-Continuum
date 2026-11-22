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
     * @param vendorRefresh             Whether this title's saved vendor choice asks for fresh stock on entry. MM7
     *                                  applies it only together with Private mods.
     * @param allArtifactsStorage       Whether Private mods asks for collection-based artifact eligibility and storage support.
     * @param chestReroll               Whether Private mods allows rerolling unclaimed MM7 chest loot.
     * @param monsterLootReroll          Whether Private mods allows MM7 monster loot previews and rerolls.
     * @param mm6AllArtifacts           Whether MM6's saved choice asks for the All Artifacts mod. Only MM6 reads it.
     * @param mm6ChestReroll            Whether MM6's saved choice allows rerolling MM6 chest loot. Only MM6 reads it.
     * @param mm6MonsterLootReroll      Whether MM6's saved choice asks for Fortune's Turn. Only MM6 reads it.
     * @return                          The environment to launch the helper with.
     */
    func environment(base: [String: String], modRoot: URL?, vendorRefresh: Bool,
                     allArtifactsStorage: Bool, chestReroll: Bool, monsterLootReroll: Bool,
                     mm6AllArtifacts: Bool = false, mm6ChestReroll: Bool = false,
                     mm6MonsterLootReroll: Bool = false) -> [String: String] {
        var environment = base
        environment.removeValue(forKey: "ENROTH_MM7_MOD_DIR")
        environment.removeValue(forKey: "ENROTH_MM7_ALL_ARTIFACTS_STORAGE")
        environment.removeValue(forKey: "ENROTH_MM7_CHEST_REROLL")
        environment.removeValue(forKey: "ENROTH_MM7_MONSTER_LOOT_REROLL")
        environment.removeValue(forKey: "ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY")
        environment.removeValue(forKey: "ENROTH_MM6_ALL_ARTIFACTS")
        environment.removeValue(forKey: "ENROTH_MM6_CHEST_REROLL")
        environment.removeValue(forKey: "ENROTH_MM6_MONSTER_LOOT_REROLL")
        switch self {
        case .mm6:
            environment.removeValue(forKey: "ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY")
            environment.removeValue(forKey: "OPENENROTH_MM7_PATH")
            environment["ENROTH_MM6_VENDOR_REFRESH_ON_ENTRY"] = vendorRefresh ? "1" : "0"
            environment["ENROTH_MM6_ALL_ARTIFACTS"] = mm6AllArtifacts ? "1" : "0"
            environment["ENROTH_MM6_CHEST_REROLL"] = mm6ChestReroll ? "1" : "0"
            environment["ENROTH_MM6_MONSTER_LOOT_REROLL"] = mm6MonsterLootReroll ? "1" : "0"
        case .fork:
            environment["ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY"] = "0"
            environment["ENROTH_MM7_ALL_ARTIFACTS_STORAGE"] = "0"
            environment["ENROTH_MM7_CHEST_REROLL"] = "0"
            environment["ENROTH_MM7_MONSTER_LOOT_REROLL"] = "0"
            if let modRoot {
                environment["ENROTH_MM7_MOD_DIR"] = modRoot.path
                if vendorRefresh { environment["ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY"] = "1" }
                if allArtifactsStorage { environment["ENROTH_MM7_ALL_ARTIFACTS_STORAGE"] = "1" }
                if chestReroll { environment["ENROTH_MM7_CHEST_REROLL"] = "1" }
                if monsterLootReroll { environment["ENROTH_MM7_MONSTER_LOOT_REROLL"] = "1" }
            }
        case .upstream:
            environment.removeValue(forKey: "ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY")
            environment.removeValue(forKey: "OPENENROTH_MM7_PATH") // Upstream's fallback MM7 data path.
        }
        return environment
    }
}
