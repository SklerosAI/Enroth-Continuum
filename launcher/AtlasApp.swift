import AppKit
import Carbon
import Darwin
import Foundation
import ImageIO
import UniformTypeIdentifiers
import WebKit

private let gameTitles = ["mm6", "mm7", "mm8"]
private let portraitMoods = ["normal", "talk", "smile", "sad", "hurt", "pain", "afraid", "cast",
                             "sleep", "unconscious", "weak", "cursed", "drunk", "insane", "sick",
                             "paralyzed", "stone", "zombie", "dead", "eradicated"]
private let rollingVideoHotKeyID: UInt32 = 1
private weak var rollingVideoHotKeyOwner: AtlasApp?
private let mm6RuleNames = ["creation.json", "spells.json", "first-aid.json", "power-cure.json", "timed-cures.json",
                            "combat-buffs.json", "house-rooms.json", "transport.json", "shop-stock.json"]

private func rollingVideoHotKeyHandler(_ nextHandler: EventHandlerCallRef?, _ event: EventRef?,
                                       _ userData: UnsafeMutableRawPointer?) -> OSStatus {
    var hotKey = EventHotKeyID()
    let status = GetEventParameter(event, EventParamName(kEventParamDirectObject),
                                   EventParamType(kEventParamDirectObject), nil,
                                   MemoryLayout<EventHotKeyID>.size, nil, &hotKey)
    if status == noErr && hotKey.id == rollingVideoHotKeyID {
        DispatchQueue.main.async { rollingVideoHotKeyOwner?.saveRollingVideoClip() }
    }
    return noErr
}

final class AtlasApp: NSObject, NSApplicationDelegate, WKScriptMessageHandler, WKNavigationDelegate {
    private var window: NSWindow!
    private var webView: WKWebView!
    private var game: Process?
    private var gameBuild: GameBuild?
    private var recorder: Process?
    private var recordingSession: URL?
    private var recordingMonitor: Timer?
    private var recordingFinalizing = false
    private var recordingStartFailed = false
    private var storageWarningShown = false
    private var recorderExitWarningShown = false
    private var rollingVideoHotKey: EventHotKeyRef?
    private var reportedWarningLines = 0
    private var memoryWarningShown = false
    private let preferences = UserDefaults.standard
    private var portraitPreviewCache: [URL: (Date, String)] = [:]

    private var supportPath: URL {
        FileManager.default.homeDirectoryForCurrentUser
            .appendingPathComponent("Library/Application Support/Enroth Continuum", isDirectory: true)
    }

    private func resolvedUserPath() -> URL? {
        MM7ProfilePath.resolve(environment: ProcessInfo.processInfo.environment, supportPath: supportPath)
    }

    private var modRoot: URL {
        supportPath.appendingPathComponent("Mods/MM7", isDirectory: true)
    }

    /**
     * Atlas profiles are isolated from the MM6 integration profile, but the current MM6 runtime needs its local rule
     * JSON. Seed only missing files so a player-owned Atlas profile is never overwritten.
     */
    private func seedMM6Profile(_ profilePath: URL) throws {
        let fileManager = FileManager.default
        let source = supportPath.appendingPathComponent("Profiles/MM6", isDirectory: true)
        let sourceRules = source.appendingPathComponent("rules", isDirectory: true)
        let targetRules = profilePath.appendingPathComponent("rules", isDirectory: true)
        var isDirectory: ObjCBool = false
        guard fileManager.fileExists(atPath: sourceRules.path, isDirectory: &isDirectory), isDirectory.boolValue else {
            throw NSError(domain: "AtlasMM6Profile", code: 1, userInfo: [NSLocalizedDescriptionKey:
                "The local MM6 rule set is missing. Restore Profiles/MM6/rules before launching MM6."])
        }
        try fileManager.createDirectory(at: targetRules, withIntermediateDirectories: true)
        for name in mm6RuleNames {
            let input = sourceRules.appendingPathComponent(name)
            let output = targetRules.appendingPathComponent(name)
            guard fileManager.fileExists(atPath: input.path) else {
                throw NSError(domain: "AtlasMM6Profile", code: 2, userInfo: [NSLocalizedDescriptionKey:
                    "The local MM6 rule set is incomplete (missing \(name))."])
            }
            if !fileManager.fileExists(atPath: output.path) {
                try fileManager.copyItem(at: input, to: output)
            }
        }
        let sourceConfig = source.appendingPathComponent("openenroth.ini")
        let targetConfig = profilePath.appendingPathComponent("openenroth.ini")
        if fileManager.fileExists(atPath: sourceConfig.path), !fileManager.fileExists(atPath: targetConfig.path) {
            try fileManager.copyItem(at: sourceConfig, to: targetConfig)
        }
        try setMM6AtlasWindowDefaults(targetConfig)
    }

    /**
     * The private integration profile can legitimately be borderless for development, but that mode is a poor
     * default for the independent Atlas profile on Retina displays: it makes the fixed 640x480 creation UI look
     * like a full-display overlay and can leave its pointer mapping ambiguous. Keep that native 4:3 frame fixed.
     * Apply this migration once only;
     * afterwards the Atlas player owns the setting.
     */
    private func setMM6AtlasWindowDefaults(_ configPath: URL) throws {
        let versionKey = "mm6.profile.windowDefaults.version"
        guard preferences.integer(forKey: versionKey) < 2,
              FileManager.default.fileExists(atPath: configPath.path) else { return }
        let contents = try String(contentsOf: configPath, encoding: .utf8)
        var inWindowSection = false
        var replacedMode = false
        let updated = contents.split(separator: "\n", omittingEmptySubsequences: false).map { line -> String in
            let trimmed = line.trimmingCharacters(in: .whitespaces)
            if trimmed.hasPrefix("[") && trimmed.hasSuffix("]") {
                inWindowSection = trimmed == "[window]"
            }
            if inWindowSection {
                for (key, value) in [("mode", "windowed"), ("resizable", "false")] {
                    if trimmed.hasPrefix(key) && trimmed.dropFirst(key.count).trimmingCharacters(in: .whitespaces).hasPrefix("=") {
                        replacedMode = true
                        return "\(key) = \(value)"
                    }
                }
            }
            return String(line)
        }.joined(separator: "\n")
        if replacedMode {
            try updated.write(to: configPath, atomically: true, encoding: .utf8)
        }
        preferences.set(1, forKey: versionKey)
    }

    private var recordingBasePath: URL {
        if let chosen = preferences.string(forKey: "recording.mm7.path") {
            return URL(fileURLWithPath: chosen, isDirectory: true)
        }
        return supportPath.appendingPathComponent("Recordings/MM7", isDirectory: true)
    }

    /** Both toggles default to the single recording switch of earlier Atlas versions. */
    private var recordVideo: Bool {
        preferences.object(forKey: "recording.mm7.video") == nil
            ? preferences.bool(forKey: "recording.mm7.enabled") : preferences.bool(forKey: "recording.mm7.video")
    }

    private var recordDiagnostics: Bool {
        preferences.object(forKey: "recording.mm7.diagnostics") == nil
            ? preferences.bool(forKey: "recording.mm7.enabled") : preferences.bool(forKey: "recording.mm7.diagnostics")
    }

    private var recordingEnabled: Bool { recordVideo || recordDiagnostics }

    private func setRecording(_ key: String, _ enabled: Bool) {
        preferences.set(recordVideo, forKey: "recording.mm7.video")
        preferences.set(recordDiagnostics, forKey: "recording.mm7.diagnostics")
        preferences.set(enabled, forKey: key)
        let video = recordVideo ? "Video recording is on" : "Video recording is off"
        let diagnostics = recordDiagnostics ? "diagnostics are on" : "diagnostics are off"
        sendState(message: "\(video) and \(diagnostics) for the next launch.")
    }

    private var recordingScript: URL? {
        Bundle.main.url(forResource: "mm7_playtest_capture", withExtension: "sh", subdirectory: "Atlas")
    }

    private func dataPath(for title: String) -> URL? {
        if let saved = preferences.string(forKey: "data.\(title)") {
            return URL(fileURLWithPath: saved, isDirectory: true)
        }
        if title == "mm7" {
            return supportPath.appendingPathComponent("Game Data/MM7", isDirectory: true)
        }
        if title == "mm6" {
            return FileManager.default.homeDirectoryForCurrentUser
                .appendingPathComponent("Library/Application Support/Enroth Continuum/Game Data/MM6/app", isDirectory: true)
        }
        if title == "mm8" {
            return FileManager.default.homeDirectoryForCurrentUser
                .appendingPathComponent("Library/Application Support/Enroth Continuum/Game Data/MM8", isDirectory: true)
        }
        return nil
    }

    private func dataReady(_ title: String, at selectedPath: URL? = nil) -> Bool {
        guard let url = selectedPath ?? dataPath(for: title) else { return false }
        let expected: [String]
        switch title {
        case "mm6": expected = ["anims/anims1.vid", "data/games.lod", "data/icons.lod", "sounds/audio.snd"]
        case "mm7": expected = ["anims/magic7.vid", "anims/might7.vid", "data/bitmaps.lod",
                                "data/events.lod", "data/games.lod", "data/icons.lod",
                                "data/sprites.lod", "sounds/audio.snd"]
        case "mm8": expected = ["anims/magicdod.vid", "data/games.lod", "data/icons.lod"]
        default: return false
        }
        return expected.allSatisfy { component in
            let parts = component.split(separator: "/").map(String.init)
            guard let folder = try? FileManager.default.contentsOfDirectory(atPath: url.path),
                  let matchedFolder = folder.first(where: { $0.lowercased() == parts[0] }),
                  let files = try? FileManager.default.contentsOfDirectory(atPath: url.appendingPathComponent(matchedFolder).path) else { return false }
            guard let matchedFile = files.first(where: { $0.lowercased() == parts[1] }) else { return false }
            var isDirectory: ObjCBool = false
            return FileManager.default.fileExists(atPath: url.appendingPathComponent(matchedFolder)
                .appendingPathComponent(matchedFile).path, isDirectory: &isDirectory) && !isDirectory.boolValue
        }
    }

    func applicationDidFinishLaunching(_ notification: Notification) {
        var eventType = EventTypeSpec(eventClass: OSType(kEventClassKeyboard), eventKind: UInt32(kEventHotKeyPressed))
        InstallEventHandler(GetApplicationEventTarget(), rollingVideoHotKeyHandler, 1, &eventType, nil, nil)
        let configuration = WKWebViewConfiguration()
        configuration.websiteDataStore = .nonPersistent()
        configuration.userContentController.add(self, name: "atlas")

        webView = WKWebView(frame: .zero, configuration: configuration)
        webView.navigationDelegate = self
        webView.setValue(false, forKey: "drawsBackground")

        window = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 1180, height: 760),
                          styleMask: [.titled, .closable, .miniaturizable, .resizable],
                          backing: .buffered, defer: false)
        window.title = "Enroth Continuum · The Atlas"
        window.minSize = NSSize(width: 900, height: 620)
        window.contentView = webView
        window.center()
        window.makeKeyAndOrderFront(nil)
        NSApp.activate(ignoringOtherApps: true)

        guard let page = Bundle.main.url(forResource: "index", withExtension: "html", subdirectory: "Atlas") else {
            showError("The Atlas interface is missing from this app.")
            return
        }
        webView.loadFileURL(page, allowingReadAccessTo: page.deletingLastPathComponent())
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }

    func webView(_ webView: WKWebView, didFinish navigation: WKNavigation!) { sendState() }

    func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction,
                 decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
        if navigationAction.request.url?.isFileURL == true { decisionHandler(.allow) }
        else { decisionHandler(.cancel) }
    }

    func userContentController(_ userContentController: WKUserContentController,
                               didReceive message: WKScriptMessage) {
        guard let action = message.body as? String else { return }
        if action == "refresh" { sendState() }
        else if action == "play-mm6" { play(.mm6) }
        else if action == "play-mm7" { play(.fork) }
        else if action == "play-openenroth" { play(.upstream) }
        else if action == "record-video-on" || action == "record-video-off" {
            guard game == nil, !recordingFinalizing else { return }
            setRecording("recording.mm7.video", action.hasSuffix("-on"))
        }
        else if action == "record-diagnostics-on" || action == "record-diagnostics-off" {
            guard game == nil, !recordingFinalizing else { return }
            setRecording("recording.mm7.diagnostics", action.hasSuffix("-on"))
        }
        else if action == "choose-record-folder" { chooseRecordingFolder() }
        else if action == "open-record-folder" {
            if FileManager.default.fileExists(atPath: recordingBasePath.path) {
                NSWorkspace.shared.open(recordingBasePath)
            } else {
                sendState(message: "The recording folder is unavailable. Reconnect the external drive or choose another folder.")
            }
        }
        else if action == "open-saves" { openSaves() }
        else if action == "open-mm7-mods" { openMM7Mods() }
        else if action == "backup-saves" { backupSaves() }
        else if action == "mods-on" { preferences.set(true, forKey: "mods.mm7.enabled"); sendState(message: "MM7 private mods will apply at the next launch.") }
        else if action == "mods-off" { preferences.set(false, forKey: "mods.mm7.enabled"); sendState(message: "Vanilla MM7 will run at the next launch.") }
        else if action == "vendor-refresh-on" { preferences.set(true, forKey: "mods.vendorRefreshOnEntry"); sendState(message: "Fresh vendor stock is saved for MM6 and MM7. MM7 applies on the next private-mods launch.") }
        else if action == "vendor-refresh-off" { preferences.set(false, forKey: "mods.vendorRefreshOnEntry"); sendState(message: "Vendor stock will use its normal refresh schedule on the next launch.") }
        else if action.hasPrefix("choose-portrait-") {
            if let slot = Int(action.dropFirst("choose-portrait-".count)), (1...4).contains(slot) { choosePortrait(slot) }
        }
        else if action.hasPrefix("clear-portrait-") {
            if let slot = Int(action.dropFirst("clear-portrait-".count)), (1...4).contains(slot) { clearPortrait(slot) }
        }
        else if action.hasPrefix("choose-mood-") || action.hasPrefix("clear-mood-") {
            let parts = action.split(separator: "-")
            if parts.count == 4, let slot = Int(parts[2]), (1...4).contains(slot) {
                let mood = String(parts[3])
                if portraitMoods.contains(mood) {
                    if parts[0] == "choose" { choosePortrait(slot, mood: mood) }
                    else { clearPortrait(slot, mood: mood) }
                }
            }
        }
        else if action.hasPrefix("import-moods-") {
            if let slot = Int(action.dropFirst("import-moods-".count)), (1...4).contains(slot) {
                importPortraitFolder(slot)
            }
        }
        else if action.hasPrefix("armor:") { saveArmor(action) }
        else if action == "clear-armor" { clearArmor() }
        else if action.hasPrefix("choose-data-") {
            let title = String(action.dropFirst("choose-data-".count))
            if gameTitles.contains(title) { chooseData(for: title) }
        }
    }

    private func saveNames() -> [[String: String]] {
        guard let profilePath = resolvedUserPath() else { return [] }
        let folder = profilePath.appendingPathComponent("saves", isDirectory: true)
        let files = (try? FileManager.default.contentsOfDirectory(at: folder,
            includingPropertiesForKeys: [.contentModificationDateKey], options: [.skipsHiddenFiles])) ?? []
        return files.filter { $0.pathExtension.lowercased() == "mm7" }
            .sorted { (try? $0.resourceValues(forKeys: [.contentModificationDateKey]).contentModificationDate ?? .distantPast) ?? .distantPast >
                      (try? $1.resourceValues(forKeys: [.contentModificationDateKey]).contentModificationDate ?? .distantPast) ?? .distantPast }
            .prefix(6).map { url in
                let date = (try? url.resourceValues(forKeys: [.contentModificationDateKey]).contentModificationDate) ?? nil
                return ["name": url.lastPathComponent,
                        "date": date.map { DateFormatter.localizedString(from: $0, dateStyle: .medium, timeStyle: .short) } ?? ""]
            }
    }

    private func sendState(message: String? = nil) {
        guard webView != nil else { return }
        var state: [String: Any] = [
            "data": Dictionary(uniqueKeysWithValues: gameTitles.map { ($0, dataReady($0)) }),
            "paths": Dictionary(uniqueKeysWithValues: gameTitles.map { ($0, dataPath(for: $0)?.path ?? "") }),
            "saves": saveNames(),
            "running": game != nil,
            "runningBuild": gameBuild?.rawValue ?? "",
            "mm6Available": GameBuild.mm6.isAvailable(in: Bundle.main.bundleURL),
            "upstreamAvailable": GameBuild.upstream.isAvailable(in: Bundle.main.bundleURL),
            "recordingFinalizing": recordingFinalizing,
            "recordingEnabled": recordingEnabled,
            "recordVideo": recordVideo,
            "recordDiagnostics": recordDiagnostics,
            "recordingPath": recordingBasePath.path,
            "recordingSession": recordingSession?.path ?? preferences.string(forKey: "recording.mm7.latestSession") ?? "",
            "modSupport": true,
            "modsEnabled": preferences.bool(forKey: "mods.mm7.enabled"),
            "vendorRefreshOnEntry": preferences.bool(forKey: "mods.vendorRefreshOnEntry"),
            "portraits": (1...4).map { validPortraitURL($0, mood: "normal") != nil },
            "portraitNormalFiles": (1...4).map { portraitExists($0, mood: "normal") },
            "legacyPortraits": (1...4).map { FileManager.default.fileExists(atPath: portraitURL($0).path) },
            "newNormalPortraits": (1...4).map { FileManager.default.fileExists(atPath: portraitURL($0, mood: "normal").path) },
            "portraitMoods": (1...4).map { slot in
                let normalReady = validPortraitURL(slot, mood: "normal") != nil
                return Dictionary(uniqueKeysWithValues: portraitMoods.map { mood in
                    let valid = validPortraitURL(slot, mood: mood) != nil
                    return (mood, ["ready": valid && normalReady,
                            "valid": valid,
                            "exists": portraitExists(slot, mood: mood),
                            "preview": portraitPreview(slot, mood: mood) ?? ""] as [String: Any])
                })
            },
            "armor": ["saved": preferences.bool(forKey: "mods.mm7.armor.saved") &&
                         FileManager.default.fileExists(atPath: modRoot.appendingPathComponent("armor.tsv").path),
                      "id": preferences.integer(forKey: "mods.mm7.armor.id"),
                      "base": preferences.integer(forKey: "mods.mm7.armor.base"),
                      "bonus": preferences.integer(forKey: "mods.mm7.armor.bonus")] as [String: Any]
        ]
        if let message { state["message"] = message }
        guard let json = try? JSONSerialization.data(withJSONObject: state),
              let source = String(data: json, encoding: .utf8) else { return }
        webView.evaluateJavaScript("window.Atlas.receive(\(source))", completionHandler: nil)
    }

    private func modWarnings(since startedAt: Date, profilePath: URL) -> [String] {
        let folder = profilePath.appendingPathComponent("logs", isDirectory: true)
        let files = (try? FileManager.default.contentsOfDirectory(at: folder,
            includingPropertiesForKeys: [.contentModificationDateKey, .fileSizeKey])) ?? []
        let recent = files.filter { file in
            guard file.lastPathComponent.hasPrefix("enroth-continuum-mm7_"), file.pathExtension == "log",
                  let values = try? file.resourceValues(forKeys: [.contentModificationDateKey, .fileSizeKey]),
                  let changed = values.contentModificationDate, changed >= startedAt.addingTimeInterval(-2),
                  let size = values.fileSize, size <= 4 * 1024 * 1024 else { return false }
            return true
        }.sorted { left, right in
            let a = (try? left.resourceValues(forKeys: [.contentModificationDateKey]).contentModificationDate) ?? .distantPast
            let b = (try? right.resourceValues(forKeys: [.contentModificationDateKey]).contentModificationDate) ?? .distantPast
            return a > b
        }
        guard let log = recent.first, let contents = try? String(contentsOf: log, encoding: .utf8) else { return [] }
        var warnings: [String] = []
        if contents.contains("Ignoring invalid MM7 portrait") {
            warnings.append("One or more portrait PNGs were ignored. Use PNG files 32–512 pixels wide and high, under 4 MB.")
        }
        if contents.contains("Ignoring private armor overrides") {
            warnings.append("The armor override was ignored. Check armor.tsv or clear it in the Atelier.")
        }
        return warnings
    }

    private func chooseRecordingFolder() {
        guard game == nil, !recordingFinalizing else { return }
        let panel = NSOpenPanel()
        panel.canChooseDirectories = true
        panel.canChooseFiles = false
        panel.canCreateDirectories = true
        panel.allowsMultipleSelection = false
        panel.prompt = "Use for recordings"
        panel.message = "Choose a local folder or an external drive folder for private MM7 recordings."
        if panel.runModal() == .OK, let folder = panel.url,
           let volumeID = recordingVolumeID(at: folder) {
            preferences.set(folder.standardizedFileURL.path, forKey: "recording.mm7.path")
            preferences.set(volumeID, forKey: "recording.mm7.volumeID")
            sendState(message: "MM7 recordings will go to the chosen folder.")
        }
    }

    private func recordingVolumeID(at folder: URL) -> String? {
        var metadata = stat()
        let result = folder.path.withCString { stat($0, &metadata) }
        return result == 0 ? String(metadata.st_dev) : nil
    }

    private func freeStorageMiB(at folder: URL) -> Int? {
        guard let attributes = try? FileManager.default.attributesOfFileSystem(forPath: folder.path),
              let bytes = attributes[.systemFreeSize] as? NSNumber else { return nil }
        return bytes.int64Value > 0 ? Int(bytes.int64Value / 1_048_576) : nil
    }

    private func recordingPreflight(profilePath: URL, dataPath: URL) -> String? {
        guard recordingScript != nil else { return "The bundled gameplay recorder is missing." }
        let folder = recordingBasePath
        if preferences.string(forKey: "recording.mm7.path") == nil {
            do { try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true) }
            catch { return "Could not create the local recording folder: \(error.localizedDescription)" }
        }
        var isDirectory: ObjCBool = false
        guard FileManager.default.fileExists(atPath: folder.path, isDirectory: &isDirectory),
              isDirectory.boolValue, FileManager.default.isWritableFile(atPath: folder.path) else {
            return "The recording folder is unavailable or unwritable. Reconnect the external drive or choose another folder."
        }
        if preferences.string(forKey: "recording.mm7.path") != nil {
            guard let selectedVolume = preferences.string(forKey: "recording.mm7.volumeID"),
                  recordingVolumeID(at: folder) == selectedVolume else {
                return "The recording drive changed or was remounted. Choose its folder again before recording."
            }
        }
        let destination = folder.resolvingSymlinksInPath().standardizedFileURL.path + "/"
        for protected in [profilePath, dataPath] {
            let path = protected.resolvingSymlinksInPath().standardizedFileURL.path + "/"
            if destination.hasPrefix(path) {
                return "Choose a recording folder outside the MM7 profile and game data folders."
            }
        }
        guard let free = freeStorageMiB(at: folder) else {
            return "Could not check free storage on the recording drive."
        }
        if free < 2_048 { return "The recording drive has less than 2 GB free. Choose another folder before recording." }
        return nil
    }

    private func showRecordingWarning(_ detail: String) {
        let alert = NSAlert()
        alert.messageText = "Gameplay recording warning"
        alert.informativeText = detail
        alert.alertStyle = .warning
        alert.addButton(withTitle: "OK")
        NSApp.activate(ignoringOtherApps: true)
        alert.runModal()
    }

    private func recordingStatus(at session: URL) -> [String: Any]? {
        let path = session.appendingPathComponent("status.json")
        guard let data = try? Data(contentsOf: path),
              let value = try? JSONSerialization.jsonObject(with: data) as? [String: Any] else { return nil }
        return value
    }

    private func startRecorder(build: GameBuild, profilePath: URL, gameProcess: Process,
                               expectedVolumeID: String) throws {
        guard let script = recordingScript else { throw NSError(domain: "AtlasRecording", code: 1,
            userInfo: [NSLocalizedDescriptionKey: "The bundled gameplay recorder is missing."]) }
        let stamp = ISO8601DateFormatter().string(from: Date()).replacingOccurrences(of: ":", with: "-")
        let session = recordingBasePath.appendingPathComponent("mm7-playtest-\(stamp)-\(UUID().uuidString.prefix(8))", isDirectory: true)
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/bin/bash")
        process.arguments = [script.path, "start", "--profile", profilePath.path,
                             "--session", session.path, "--app", Bundle.main.bundleURL.path,
                             "--game-pid", String(gameProcess.processIdentifier), "--build", build.rawValue,
                             recordVideo ? "--video" : "--no-video",
                             recordDiagnostics ? "--diagnostics" : "--no-diagnostics",
                             "--display", "1", "--video-chunk-seconds", "60", "--save-checkpoint-seconds", "600", "--min-free-mib", "2048",
                             "--expected-volume-id", expectedVolumeID]
        process.standardOutput = FileHandle.nullDevice
        process.standardError = FileHandle.nullDevice
        try process.run()
        let deadline = Date().addingTimeInterval(3)
        var ready = false
        repeat {
            if process.isRunning, recordingStatus(at: session)?["state"] as? String == "running" {
                ready = true
                break
            }
            if !process.isRunning { break }
            Thread.sleep(forTimeInterval: 0.05)
        } while Date() < deadline
        guard ready else {
            if process.isRunning { _ = Darwin.kill(process.processIdentifier, SIGTERM) }
            throw NSError(domain: "AtlasRecording", code: 2,
                userInfo: [NSLocalizedDescriptionKey: "The recorder did not create a ready session. Check the destination and try again."])
        }
        recorder = process
        recordingSession = session
        if recordVideo { registerRollingVideoHotKey() }
        preferences.set(session.path, forKey: "recording.mm7.latestSession")
        storageWarningShown = false
        recorderExitWarningShown = false
        reportedWarningLines = 0
        memoryWarningShown = false
        recordingMonitor?.invalidate()
        recordingMonitor = Timer.scheduledTimer(withTimeInterval: 20, repeats: true) { [weak self] _ in
            self?.checkRecordingHealth()
        }
    }

    private func registerRollingVideoHotKey() {
        guard rollingVideoHotKey == nil else { return }
        let hotKeyID = EventHotKeyID(signature: OSType(0x4543_5243), id: rollingVideoHotKeyID)
        var hotKey: EventHotKeyRef?
        let status = RegisterEventHotKey(UInt32(kVK_ANSI_R), UInt32(cmdKey | shiftKey), hotKeyID,
                                         GetApplicationEventTarget(), 0, &hotKey)
        guard status == noErr else {
            showRecordingWarning("Could not register the rolling-video hotkey. Logs and save checkpoints continue.")
            return
        }
        rollingVideoHotKeyOwner = self
        rollingVideoHotKey = hotKey
    }

    private func unregisterRollingVideoHotKey() {
        if let rollingVideoHotKey { UnregisterEventHotKey(rollingVideoHotKey) }
        rollingVideoHotKey = nil
        if rollingVideoHotKeyOwner === self { rollingVideoHotKeyOwner = nil }
    }

    fileprivate func saveRollingVideoClip() {
        guard game?.isRunning == true, recorder?.isRunning == true, let session = recordingSession,
              let script = recordingScript else { return }
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/bin/bash")
        process.arguments = [script.path, "clip", "--session", session.path]
        process.standardOutput = FileHandle.nullDevice
        process.standardError = FileHandle.nullDevice
        guard (try? process.run()) != nil else { return }
        NSSound.beep()
        sendState(message: "Saving the last two minutes of video. The clip is complete within a minute.")
    }

    private func requestRecorderStop(session: URL, normalExit: Bool, exitDetail: String) {
        guard let script = recordingScript else { return }
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/bin/bash")
        process.arguments = [script.path, "stop", "--session", session.path,
                             "--game-exit", normalExit ? "normal" : "abnormal", "--exit-detail", exitDetail]
        process.standardOutput = FileHandle.nullDevice
        process.standardError = FileHandle.nullDevice
        try? process.run()
    }

    private func reportFinishedGame(_ finished: Process, build: GameBuild, session: URL?, recorderProcess: Process?,
                                    warningText: String, attempt: Int = 0) {
        if recorderProcess?.isRunning == true {
            if attempt == 90 { // The recorder can wait up to 75 s for the last video chunk of a crash clip.
                if let recorderProcess { _ = Darwin.kill(recorderProcess.processIdentifier, SIGTERM) }
                sendState(message: "The recorder is taking longer than expected to finish, so Atlas is stopping it.")
            }
            if attempt == 100, let recorderProcess {
                _ = Darwin.kill(recorderProcess.processIdentifier, SIGKILL)
            }
            if attempt >= 105 {
                recordingFinalizing = false
                guard game == nil else { return }
                sendState(message: "\(build.displayName) closed. The recorder did not finish cleanly. Inspect its private session folder before relying on the capture." + warningText)
                return
            }
            DispatchQueue.main.asyncAfter(deadline: .now() + 1) { [weak self] in
                self?.reportFinishedGame(finished, build: build, session: session, recorderProcess: recorderProcess,
                                         warningText: warningText, attempt: attempt + 1)
            }
            return
        }
        recordingFinalizing = false
        guard game == nil else { return } // A newer MM7 launch owns the Atlas state.
        let normalExit = finished.terminationReason == .exit && finished.terminationStatus == 0
        var message = normalExit ? "Returned from \(build.displayName). Your saves remain on this Mac."
                                 : "\(build.displayName) exited unexpectedly. Check your latest save before continuing."
        if let session {
            message += " Recording session: \(session.path)"
            if let status = recordingStatus(at: session) {
                let state = status["state"] as? String ?? "unknown"
                let video = status["video"] as? String ?? "unknown"
                if state != "stopped" && state != "game-exited" {
                    message += " Recording ended with \(state); inspect status.json and warnings.txt."
                }
                if video == "failed" {
                    message += " Screen video failed. Check Screen Recording permission for Enroth Continuum in System Settings."
                }
                if let clips = status["clips"] as? Int, clips > 0 {
                    message += " Saved \(clips) video clip\(clips == 1 ? "" : "s") in its clips folder."
                }
                if FileManager.default.fileExists(atPath: session.appendingPathComponent("warnings.txt").path) {
                    message += " Check warnings.txt before relying on the capture."
                }
                if let storage = status["storage_warnings"] as? [String], !storage.isEmpty {
                    message += " " + storage.joined(separator: " ")
                }
            } else {
                message += " Recording completion was not confirmed; inspect the session folder."
            }
            if recorderProcess?.terminationStatus != 0 && recorderProcess?.isRunning == false {
                message += " The recorder process exited with an error."
            }
        }
        sendState(message: message + warningText)
    }

    private func checkRecordingHealth() {
        guard game?.isRunning == true, let session = recordingSession else { return }
        let free = freeStorageMiB(at: recordingBasePath)
        if !storageWarningShown {
            if free == nil {
                storageWarningShown = true
                showRecordingWarning("The recording drive is unavailable. MM7 can continue, but the recorder may have stopped.")
            } else if let free, free < 4_096 {
                storageWarningShown = true
                showRecordingWarning("The recording drive has under 4 GB free. The recorder will stop at 2 GB to protect the drive; MM7 can continue.")
            }
        }
        if !recorderExitWarningShown, recorder?.isRunning != true {
            recorderExitWarningShown = true
            showRecordingWarning("The recorder stopped before the game closed. Check the session folder before relying on this recording.")
        }
        let warningFile = session.appendingPathComponent("warnings.txt")
        if let contents = try? String(contentsOf: warningFile, encoding: .utf8) {
            let lines = contents.split(separator: "\n").count
            if lines > reportedWarningLines {
                reportedWarningLines = lines
                showRecordingWarning("The recorder reported a new problem. Check warnings.txt in the session folder; MM7 can continue.")
            }
        }
        if !memoryWarningShown {
            DispatchQueue.global(qos: .utility).async { [weak self] in
                guard let self, let percent = self.memoryFreePercent(), percent < 10 else { return }
                DispatchQueue.main.async {
                    guard self.game?.isRunning == true, !self.memoryWarningShown else { return }
                    self.memoryWarningShown = true
                    self.showRecordingWarning("System memory is below 10% free. Save your game soon and close other apps if performance drops.")
                }
            }
        }
    }

    private func memoryFreePercent() -> Int? {
        let process = Process()
        process.executableURL = URL(fileURLWithPath: "/usr/bin/memory_pressure")
        process.arguments = ["-Q"]
        let output = Pipe()
        process.standardOutput = output
        process.standardError = FileHandle.nullDevice
        guard (try? process.run()) != nil else { return nil }
        let data = output.fileHandleForReading.readDataToEndOfFile()
        process.waitUntilExit()
        guard process.terminationStatus == 0,
              let text = String(data: data, encoding: .utf8),
              let match = text.range(of: "System-wide memory free percentage: ") else { return nil }
        let tail = text[match.upperBound...]
        return Int(tail.prefix(while: { $0.isNumber }))
    }

    private func play(_ build: GameBuild) {
        let name = build.displayName
        guard let profilePath = build.profilePath(environment: ProcessInfo.processInfo.environment,
                                                  supportPath: supportPath) else {
            sendState(message: "The \(name) profile is invalid; \(name) was not launched.")
            return
        }
        guard game == nil else {
            if let game, game.isRunning { NSRunningApplication(processIdentifier: game.processIdentifier)?.activate() }
            sendState(message: "\(gameBuild?.displayName ?? "MM7") is already running or finishing its exit.")
            return
        }
        guard !recordingFinalizing else {
            sendState(message: "The previous recording is still finalizing. Play will be available when it finishes.")
            return
        }
        guard dataReady(build.dataTitle), let dataPath = dataPath(for: build.dataTitle) else {
            sendState(message: "Choose a folder containing your \(build.dataTitle.uppercased()) ANIMS, DATA, MUSIC and SOUNDS directories.")
            return
        }
        let engine = build.helperExecutable(in: Bundle.main.bundleURL)
        guard FileManager.default.isExecutableFile(atPath: engine.path) else {
            sendState(message: "The bundled \(name) engine is missing.")
            return
        }
        let shouldRecord = build.supportsRecording && recordingEnabled
        if shouldRecord, let issue = recordingPreflight(profilePath: profilePath, dataPath: dataPath) {
            sendState(message: issue)
            return
        }
        let expectedVolumeID = shouldRecord
            ? (preferences.string(forKey: "recording.mm7.path") != nil
                ? preferences.string(forKey: "recording.mm7.volumeID")
                : recordingVolumeID(at: recordingBasePath)) : nil
        if shouldRecord && expectedVolumeID == nil {
            sendState(message: "Could not verify the recording drive. Choose the folder again.")
            return
        }
        do {
            try FileManager.default.createDirectory(at: profilePath, withIntermediateDirectories: true)
            if build == .mm6 { try seedMM6Profile(profilePath) }
            let process = Process()
            process.executableURL = engine
            process.arguments = build.arguments(dataPath: dataPath, profilePath: profilePath)
            let modsEnabled = build == .fork && preferences.bool(forKey: "mods.mm7.enabled")
            if modsEnabled { try FileManager.default.createDirectory(at: modRoot, withIntermediateDirectories: true) }
            process.environment = build.environment(base: ProcessInfo.processInfo.environment,
                                                    modRoot: modsEnabled ? modRoot : nil,
                                                    vendorRefresh: preferences.bool(forKey: "mods.vendorRefreshOnEntry"))
            let startedAt = Date()
            process.terminationHandler = { [weak self] finished in
                DispatchQueue.main.async {
                    guard let self, self.game === finished else { return }
                    let session = self.recordingSession
                    let recorderProcess = self.recorder
                    let recordingStartFailed = self.recordingStartFailed
                    let videoWasRecording = self.rollingVideoHotKey != nil
                    self.recordingMonitor?.invalidate()
                    self.recordingMonitor = nil
                    self.unregisterRollingVideoHotKey()
                    self.game = nil
                    self.gameBuild = nil
                    self.recorder = nil
                    self.recordingSession = nil
                    self.recordingStartFailed = false
                    self.recordingFinalizing = session != nil
                    let normalExit = finished.terminationReason == .exit && finished.terminationStatus == 0
                    let exitDetail = finished.terminationReason == .uncaughtSignal
                        ? "uncaught signal \(finished.terminationStatus)" : "exit status \(finished.terminationStatus)"
                    if let session {
                        self.requestRecorderStop(session: session, normalExit: normalExit, exitDetail: exitDetail)
                    }
                    self.window.makeKeyAndOrderFront(nil)
                    NSApp.activate(ignoringOtherApps: true)
                    let warnings = build == .fork ? self.modWarnings(since: startedAt, profilePath: profilePath) : []
                    let warningText = (warnings.isEmpty ? "" : "\n" + warnings.joined(separator: "\n"))
                        + (recordingStartFailed ? "\nGameplay recording did not start for this session." : "")
                    if session != nil {
                        self.sendState(message: normalExit || !videoWasRecording
                                       ? "\(name) closed. Finalizing the recording…"
                                       : "\(name) exited unexpectedly. Saving the last two minutes of video…")
                    }
                    self.reportFinishedGame(finished, build: build, session: session, recorderProcess: recorderProcess,
                                            warningText: warningText)
                }
            }
            try process.run()
            game = process
            gameBuild = build
            recordingMonitor?.invalidate()
            recordingMonitor = nil
            recorder = nil
            recordingSession = nil
            recordingStartFailed = false
            if shouldRecord, let expectedVolumeID {
                do {
                    try startRecorder(build: build, profilePath: profilePath, gameProcess: process,
                                      expectedVolumeID: expectedVolumeID)
                } catch {
                    recordingStartFailed = true
                    showRecordingWarning("\(name) opened, but recording could not start: \(error.localizedDescription)")
                }
            }
            sendState(message: shouldRecord && recorder?.isRunning == true
                      ? "Opening \(name); recording is starting…"
                      : build.supportsRecording ? "Opening \(name)…" : "Opening \(name) in its separate MM6 profile…")
            window.orderOut(nil)
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.8) {
                NSRunningApplication(processIdentifier: process.processIdentifier)?.activate()
            }
        } catch {
            sendState(message: "Could not open \(name): \(error.localizedDescription)")
        }
    }

    private func openSaves() {
        guard let profilePath = resolvedUserPath() else {
            sendState(message: "The disposable MM7 profile is invalid; saves were not opened.")
            return
        }
        NSWorkspace.shared.open(profilePath.appendingPathComponent("saves", isDirectory: true))
    }

    private func chooseData(for title: String) {
        let panel = NSOpenPanel()
        panel.canChooseDirectories = true
        panel.canChooseFiles = false
        panel.allowsMultipleSelection = false
        panel.message = "Choose the \(title.uppercased()) extracted game data folder."
        if panel.runModal() == .OK, let url = panel.url {
            if dataReady(title, at: url) {
                preferences.set(url.path, forKey: "data.\(title)")
                sendState(message: "\(title.uppercased()) data folder is ready.")
            } else {
                sendState(message: "This folder is missing required \(title.uppercased()) archives. Your previous choice was kept.")
            }
        }
    }

    private func portraitURL(_ slot: Int, mood: String? = nil) -> URL {
        guard let mood else { return modRoot.appendingPathComponent("portrait-\(slot).png") }
        return modRoot.appendingPathComponent("portraits/\(slot)/\(mood).png")
    }

    private func portraitExists(_ slot: Int, mood: String) -> Bool {
        FileManager.default.fileExists(atPath: portraitURL(slot, mood: mood).path) ||
            (mood == "normal" && FileManager.default.fileExists(atPath: portraitURL(slot).path))
    }

    private func validPortraitURL(_ slot: Int, mood: String) -> URL? {
        var candidates = [portraitURL(slot, mood: mood)]
        if mood == "normal" { candidates.append(portraitURL(slot)) }
        for url in candidates {
            guard let values = try? url.resourceValues(forKeys: [.fileSizeKey]),
                  let size = values.fileSize, size > 0, size <= 4 * 1024 * 1024,
                  let source = CGImageSourceCreateWithURL(url as CFURL, [kCGImageSourceShouldCache: false] as CFDictionary),
                  (CGImageSourceGetType(source) as String?) == UTType.png.identifier,
                  let properties = CGImageSourceCopyPropertiesAtIndex(source, 0, nil) as? [CFString: Any],
                  let width = properties[kCGImagePropertyPixelWidth] as? Int,
                  let height = properties[kCGImagePropertyPixelHeight] as? Int,
                  (32...512).contains(width), (32...512).contains(height) else { continue }
            return url
        }
        return nil
    }

    private func portraitPreview(_ slot: Int, mood: String) -> String? {
        guard let url = validPortraitURL(slot, mood: mood),
              let changed = (try? url.resourceValues(forKeys: [.contentModificationDateKey]))?.contentModificationDate else { return nil }
        if let cached = portraitPreviewCache[url], cached.0 == changed { return cached.1 }
        guard let image = NSImage(contentsOf: url) else { return nil }
        guard let bitmap = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: 48, pixelsHigh: 48,
                                            bitsPerSample: 8, samplesPerPixel: 4, hasAlpha: true,
                                            isPlanar: false, colorSpaceName: .deviceRGB,
                                            bytesPerRow: 0, bitsPerPixel: 0),
              let context = NSGraphicsContext(bitmapImageRep: bitmap) else { return nil }
        if let pixels = bitmap.bitmapData {
            memset(pixels, 0, bitmap.bytesPerRow * bitmap.pixelsHigh)
        }
        NSGraphicsContext.saveGraphicsState()
        NSGraphicsContext.current = context
        image.draw(in: NSRect(x: 0, y: 0, width: 48, height: 48))
        context.flushGraphics()
        NSGraphicsContext.restoreGraphicsState()
        guard let data = bitmap.representation(using: .png, properties: [:]) else { return nil }
        let preview = "data:image/png;base64," + data.base64EncodedString()
        portraitPreviewCache[url] = (changed, preview)
        return preview
    }

    private func openMM7Mods() {
        do {
            try FileManager.default.createDirectory(at: modRoot, withIntermediateDirectories: true)
            try createModGuideIfNeeded()
            if NSWorkspace.shared.open(modRoot) {
                sendState(message: "Opened MM7 mods. README.md describes supported local files and armor.tsv can hold up to 32 armor overrides.")
            } else {
                sendState(message: "Could not open the local MM7 mods folder in Finder.")
            }
        } catch {
            sendState(message: "Could not open MM7 mods folder: \(error.localizedDescription)")
        }
    }

    private func createModGuideIfNeeded() throws {
        let guide = modRoot.appendingPathComponent("README.md")
        guard !FileManager.default.fileExists(atPath: guide.path) else { return }
        let contents = """
        # Enroth Continuum MM7 local mods

        This folder is local to this Mac. It is not copied into game archives,
        saves, the Enroth Continuum repository, or cloud services. Enable
        **Private mods** in The Atlas before launching MM7 for these files to apply.

        ## Portraits

        Put a PNG for each party position at `portraits/<slot>/normal.png`, where
        `<slot>` is 1 through 4. Optional mood PNGs use the same folder, for
        example `portraits/1/talk.png` or `portraits/1/hurt.png`. Images must be
        PNG, 32 to 512 pixels on each side, and at most 4 MB. The Atelier can
        import and validate these files. Legacy `portrait-1.png` through
        `portrait-4.png` remain supported.

        ## Armor templates

        `armor.tsv` can contain up to 32 overrides. Each non-comment row has
        exactly three tab-separated fields: item ID, base AC, and bonus AC.
        Values must be armor item IDs and AC values from 0 through 100. Invalid
        rows cause the engine to ignore the whole file and retain original values.

        ```text
        # item_id<TAB>base_ac<TAB>bonus_ac
        66<TAB>20<TAB>3
        76<TAB>35<TAB>5
        ```

        Other files are currently ignored by the engine. Keep original game data
        and existing saves outside this folder.
        """
        try contents.write(to: guide, atomically: true, encoding: .utf8)
    }

    private func choosePortrait(_ slot: Int, mood: String? = nil) {
        let panel = NSOpenPanel()
        panel.allowedContentTypes = [.png]
        panel.canChooseDirectories = false
        panel.allowsMultipleSelection = false
        panel.message = "Choose a PNG \(mood ?? "normal") portrait for MM7 party member \(slot)."
        guard panel.runModal() == .OK, let source = panel.url else { return }
        do {
            guard let png = normalizedPortraitPNG(from: source) else {
                sendState(message: "Choose a valid PNG between 32 and 512 pixels in each dimension, under 4 MB.")
                return
            }
            let destination = portraitURL(slot, mood: mood)
            try FileManager.default.createDirectory(at: destination.deletingLastPathComponent(), withIntermediateDirectories: true)
            try png.write(to: destination, options: .atomic)
            sendState(message: "Portrait \(slot) \(mood ?? "normal") is ready for the next modded MM7 launch.")
        } catch {
            sendState(message: "Could not add portrait: \(error.localizedDescription)")
        }
    }

    private func normalizedPortraitPNG(from source: URL) -> Data? {
        guard let values = try? source.resourceValues(forKeys: [.fileSizeKey]),
              let size = values.fileSize, size > 0, size <= 4 * 1024 * 1024,
              let imageSource = CGImageSourceCreateWithURL(source as CFURL,
                  [kCGImageSourceShouldCache: false] as CFDictionary),
              (CGImageSourceGetType(imageSource) as String?) == UTType.png.identifier,
              let properties = CGImageSourceCopyPropertiesAtIndex(imageSource, 0, nil) as? [CFString: Any],
              let width = properties[kCGImagePropertyPixelWidth] as? Int,
              let height = properties[kCGImagePropertyPixelHeight] as? Int,
              (32...512).contains(width), (32...512).contains(height),
              let data = try? Data(contentsOf: source),
              let image = NSBitmapImageRep(data: data),
              let png = image.representation(using: .png, properties: [:]),
              png.count <= 4 * 1024 * 1024 else { return nil }
        return png
    }

    private func importPortraitFolder(_ slot: Int) {
        let panel = NSOpenPanel()
        panel.canChooseDirectories = true
        panel.canChooseFiles = false
        panel.allowsMultipleSelection = false
        panel.message = "Choose a folder of named MM7 mood PNGs for party member \(slot)."
        guard panel.runModal() == .OK, let folder = panel.url else { return }
        do {
            let names = try FileManager.default.contentsOfDirectory(atPath: folder.path)
            let lookup = Dictionary(names.map { ($0.lowercased(), $0) }, uniquingKeysWith: { first, _ in first })
            let targetFolder = modRoot.appendingPathComponent("portraits/\(slot)", isDirectory: true)
            var imported = 0
            var rejected = 0
            for mood in portraitMoods {
                guard let name = lookup["\(mood).png"] else { continue }
                let source = folder.appendingPathComponent(name)
                guard let png = normalizedPortraitPNG(from: source) else { rejected += 1; continue }
                do {
                    try FileManager.default.createDirectory(at: targetFolder, withIntermediateDirectories: true)
                    try png.write(to: targetFolder.appendingPathComponent("\(mood).png"), options: .atomic)
                    imported += 1
                } catch {
                    rejected += 1
                }
            }
            sendState(message: "Imported \(imported) mood PNGs for party member \(slot). \(rejected) invalid files skipped. Changes apply next launch.")
        } catch {
            sendState(message: "Could not import portrait folder: \(error.localizedDescription)")
        }
    }

    private func clearPortrait(_ slot: Int, mood: String? = nil) {
        do {
            let destination = portraitURL(slot, mood: mood)
            if FileManager.default.fileExists(atPath: destination.path) {
                try FileManager.default.removeItem(at: destination)
            }
            sendState(message: "Portrait \(slot) \(mood ?? "normal") cleared for the next MM7 launch.")
        } catch {
            sendState(message: "Could not remove portrait: \(error.localizedDescription)")
        }
    }

    private func saveArmor(_ action: String) {
        let fields = action.split(separator: ":", omittingEmptySubsequences: false)
        guard fields.count == 4, let id = Int(fields[1]), let base = Int(fields[2]), let bonus = Int(fields[3]),
              (66...78).contains(id), (0...100).contains(base), (0...100).contains(bonus) else {
            sendState(message: "Armor needs an item from the list and AC values from 0 to 100.")
            return
        }
        do {
            try FileManager.default.createDirectory(at: modRoot, withIntermediateDirectories: true)
            try "# item_id\tbase_ac\tbonus_ac\n\(id)\t\(base)\t\(bonus)\n"
                .write(to: modRoot.appendingPathComponent("armor.tsv"), atomically: true, encoding: .utf8)
            preferences.set(id, forKey: "mods.mm7.armor.id")
            preferences.set(base, forKey: "mods.mm7.armor.base")
            preferences.set(bonus, forKey: "mods.mm7.armor.bonus")
            preferences.set(true, forKey: "mods.mm7.armor.saved")
            sendState(message: "Armor template \(id) will use AC \(base) + \(bonus) on the next modded launch. Every copy changes.")
        } catch {
            sendState(message: "Could not save armor values: \(error.localizedDescription)")
        }
    }

    private func clearArmor() {
        do {
            let file = modRoot.appendingPathComponent("armor.tsv")
            if FileManager.default.fileExists(atPath: file.path) {
                try FileManager.default.removeItem(at: file)
            }
            preferences.removeObject(forKey: "mods.mm7.armor.saved")
            preferences.removeObject(forKey: "mods.mm7.armor.id")
            preferences.removeObject(forKey: "mods.mm7.armor.base")
            preferences.removeObject(forKey: "mods.mm7.armor.bonus")
            sendState(message: "Original armor AC will apply on the next MM7 launch.")
        } catch {
            sendState(message: "Could not remove armor override: \(error.localizedDescription)")
        }
    }

    private func backupSaves() {
        guard let profilePath = resolvedUserPath() else {
            sendState(message: "The disposable MM7 profile is invalid; saves were not backed up.")
            return
        }
        let source = profilePath.appendingPathComponent("saves", isDirectory: true)
        guard FileManager.default.fileExists(atPath: source.path) else {
            sendState(message: "There are no saves to back up yet.")
            return
        }
        let stamp = ISO8601DateFormatter().string(from: Date()).replacingOccurrences(of: ":", with: "-")
        let destination = FileManager.default.homeDirectoryForCurrentUser
            .appendingPathComponent("Library/Application Support/Enroth Continuum/Backups/MM7-\(stamp)", isDirectory: true)
        do {
            try FileManager.default.createDirectory(at: destination.deletingLastPathComponent(), withIntermediateDirectories: true)
            try FileManager.default.copyItem(at: source, to: destination)
            sendState(message: "Saved a local MM7 backup in Enroth Continuum/Backups.")
        } catch {
            sendState(message: "Backup failed: \(error.localizedDescription)")
        }
    }

    private func showError(_ text: String) {
        let alert = NSAlert()
        alert.messageText = text
        alert.runModal()
    }
}

@main
struct AtlasEntry {
    static func main() {
        let app = NSApplication.shared
        let delegate = AtlasApp()
        app.delegate = delegate
        app.setActivationPolicy(.regular)
        app.run()
    }
}
