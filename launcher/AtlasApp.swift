import AppKit
import Foundation
import UniformTypeIdentifiers
import WebKit

private let gameTitles = ["mm6", "mm7", "mm8"]

final class AtlasApp: NSObject, NSApplicationDelegate, WKScriptMessageHandler, WKNavigationDelegate {
    private var window: NSWindow!
    private var webView: WKWebView!
    private var game: Process?
    private let preferences = UserDefaults.standard

    private var supportPath: URL {
        FileManager.default.homeDirectoryForCurrentUser
            .appendingPathComponent("Library/Application Support/Enroth Continuum", isDirectory: true)
    }

    private var userPath: URL {
        supportPath.appendingPathComponent("Profiles/MM7-Atlas", isDirectory: true)
    }

    private var modRoot: URL {
        supportPath.appendingPathComponent("Mods/MM7", isDirectory: true)
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
        else if action == "play-mm7" { playMM7() }
        else if action == "open-saves" { NSWorkspace.shared.open(userPath.appendingPathComponent("saves", isDirectory: true)) }
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
        else if action.hasPrefix("armor:") { saveArmor(action) }
        else if action == "clear-armor" { clearArmor() }
        else if action.hasPrefix("choose-data-") {
            let title = String(action.dropFirst("choose-data-".count))
            if gameTitles.contains(title) { chooseData(for: title) }
        }
    }

    private func saveNames() -> [[String: String]] {
        let folder = userPath.appendingPathComponent("saves", isDirectory: true)
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
            "running": game?.isRunning ?? false,
            "modSupport": true,
            "modsEnabled": preferences.bool(forKey: "mods.mm7.enabled"),
            "vendorRefreshOnEntry": preferences.bool(forKey: "mods.vendorRefreshOnEntry"),
            "portraits": (1...4).map { FileManager.default.fileExists(atPath: portraitURL($0).path) },
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

    private func playMM7() {
        guard game?.isRunning != true else {
            if let game { NSRunningApplication(processIdentifier: game.processIdentifier)?.activate() }
            sendState(message: "MM7 is already running.")
            return
        }
        guard dataReady("mm7"), let dataPath = dataPath(for: "mm7") else {
            sendState(message: "Choose a folder containing your MM7 ANIMS, DATA, MUSIC and SOUNDS directories.")
            return
        }
        let engine = Bundle.main.bundleURL.appendingPathComponent("Contents/Helpers/MM7.app/Contents/MacOS/MM7")
        guard FileManager.default.isExecutableFile(atPath: engine.path) else {
            sendState(message: "The bundled MM7 engine is missing.")
            return
        }
        do {
            try FileManager.default.createDirectory(at: userPath, withIntermediateDirectories: true)
            let process = Process()
            process.executableURL = engine
            process.arguments = ["--game", "mm7", "--data-path", dataPath.path, "--user-path", userPath.path]
            var environment = ProcessInfo.processInfo.environment
            environment.removeValue(forKey: "ENROTH_MM7_MOD_DIR")
            environment["ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY"] = "0"
            if preferences.bool(forKey: "mods.mm7.enabled") {
                try FileManager.default.createDirectory(at: modRoot, withIntermediateDirectories: true)
                environment["ENROTH_MM7_MOD_DIR"] = modRoot.path
                if preferences.bool(forKey: "mods.vendorRefreshOnEntry") {
                    environment["ENROTH_MM7_VENDOR_REFRESH_ON_ENTRY"] = "1"
                }
            }
            process.environment = environment
            process.terminationHandler = { [weak self] finished in
                DispatchQueue.main.async {
                    self?.game = nil
                    self?.window.makeKeyAndOrderFront(nil)
                    NSApp.activate(ignoringOtherApps: true)
                    if finished.terminationReason == .exit && finished.terminationStatus == 0 {
                        self?.sendState(message: "Returned from MM7. Your saves remain on this Mac.")
                    } else {
                        self?.sendState(message: "MM7 exited unexpectedly. Check your latest save before continuing.")
                    }
                }
            }
            try process.run()
            game = process
            sendState(message: "Opening MM7…")
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.8) {
                NSRunningApplication(processIdentifier: process.processIdentifier)?.activate()
            }
        } catch {
            sendState(message: "Could not open MM7: \(error.localizedDescription)")
        }
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

    private func portraitURL(_ slot: Int) -> URL {
        modRoot.appendingPathComponent("portrait-\(slot).png")
    }

    private func openMM7Mods() {
        do {
            try FileManager.default.createDirectory(at: modRoot, withIntermediateDirectories: true)
            if NSWorkspace.shared.open(modRoot) {
                sendState(message: "Opened the local MM7 mods folder. Supported files: portrait-1.png through portrait-4.png and armor.tsv.")
            } else {
                sendState(message: "Could not open the local MM7 mods folder in Finder.")
            }
        } catch {
            sendState(message: "Could not open MM7 mods folder: \(error.localizedDescription)")
        }
    }

    private func choosePortrait(_ slot: Int) {
        let panel = NSOpenPanel()
        panel.allowedContentTypes = [.png]
        panel.canChooseDirectories = false
        panel.allowsMultipleSelection = false
        panel.message = "Choose a PNG portrait for MM7 party member \(slot)."
        guard panel.runModal() == .OK, let source = panel.url else { return }
        do {
            let data = try Data(contentsOf: source)
            guard data.count <= 4 * 1024 * 1024, let image = NSBitmapImageRep(data: data),
                  (32...512).contains(image.pixelsWide), (32...512).contains(image.pixelsHigh),
                  let png = image.representation(using: .png, properties: [:]), png.count <= 4 * 1024 * 1024 else {
                sendState(message: "Choose a valid PNG between 32 and 512 pixels in each dimension, under 4 MB.")
                return
            }
            try FileManager.default.createDirectory(at: modRoot, withIntermediateDirectories: true)
            try png.write(to: portraitURL(slot), options: .atomic)
            sendState(message: "Portrait \(slot) is ready for the next modded MM7 launch.")
        } catch {
            sendState(message: "Could not add portrait: \(error.localizedDescription)")
        }
    }

    private func clearPortrait(_ slot: Int) {
        do {
            if FileManager.default.fileExists(atPath: portraitURL(slot).path) {
                try FileManager.default.removeItem(at: portraitURL(slot))
            }
            sendState(message: "Portrait \(slot) will use the original art.")
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
        let source = userPath.appendingPathComponent("saves", isDirectory: true)
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

let app = NSApplication.shared
let delegate = AtlasApp()
app.delegate = delegate
app.setActivationPolicy(.regular)
app.run()
