// JTTY Workbench — GPL-3.0-or-later. Created 2026-10-01.
import AppKit
import SwiftUI
import UniformTypeIdentifiers

final class WorkbenchModel: ObservableObject {
    @Published var message = "CQ K1ABC CQ"
    @Published var frequency = "1500"
    @Published var tolerance = "50"
    @Published var profile = "unknown"
    @Published var recording = ""
    @Published var busy = false
    @Published var status = "Ready · recorded audio prototype"
    @Published var output = "Create a JTTY WAV file or open a recording to decode."
    @Published var failed = false
    // This developer bundle lives at workspace/build/JTTY Workbench.app.
    let root = Bundle.main.bundleURL.deletingLastPathComponent().deletingLastPathComponent()

    func chooseRecording() {
        let panel = NSOpenPanel()
        panel.allowedContentTypes = [.wav]
        panel.canChooseDirectories = false
        if panel.runModal() == .OK, let url = panel.url { recording = url.path }
    }

    func encode() {
        let panel = NSSavePanel()
        panel.allowedContentTypes = [.wav]
        panel.nameFieldStringValue = "jtty-message.wav"
        if panel.runModal() == .OK, let url = panel.url {
            run(["encode", message, url.path, "--frequency", frequency, "--profile", profile], action: "Encoding", encodedPath: url.path)
        }
    }

    func decode() {
        run(["decode", recording, "--frequency", frequency, "--tolerance", tolerance], action: "Decoding")
    }

    func run(_ arguments: [String], action: String, encodedPath: String? = nil) {
        busy = true
        failed = false
        status = "\(action)…"
        let script = root.appendingPathComponent("client/jtty.py").path
        DispatchQueue.global(qos: .userInitiated).async {
            let process = Process()
            let stdout = Pipe(), stderr = Pipe()
            process.executableURL = URL(fileURLWithPath: "/usr/bin/python3")
            process.arguments = [script] + arguments
            process.standardOutput = stdout
            process.standardError = stderr
            do {
                try process.run()
                let data = stdout.fileHandleForReading.readDataToEndOfFile()
                let errors = stderr.fileHandleForReading.readDataToEndOfFile()
                process.waitUntilExit()
                let succeeded = process.terminationStatus == 0
                var text = String(data: succeeded ? data : errors, encoding: .utf8) ?? "No output"
                if succeeded, let result = try? JSONSerialization.jsonObject(with: data) as? [String: Any] {
                    if let messages = result["messages"] as? [[String: Any]] {
                        text = messages.isEmpty ? "No JTTY messages found in this recording." : messages.map { item in
                            let complete = (item["complete"] as? Bool) == true ? "Complete" : "Partial"
                            let hz = item["frequency_hz"] as? Double ?? 0
                            let seconds = item["start_seconds"] as? Double ?? 0
                            return String(format: "%.0f Hz · %.2f s · %@\n%@", hz, seconds, complete, item["text"] as? String ?? "")
                        }.joined(separator: "\n\n")
                    } else if let message = result["message"] as? String {
                        let frames = result["frames"] as? Int ?? 0
                        let seconds = result["signal_seconds"] as? Double ?? 0
                        text = String(format: "%@\n\n%d frames · %.3f seconds of signal\n\nSaved: %@", message, frames, seconds, result["output"] as? String ?? "")
                    }
                }
                let finishedText = text
                DispatchQueue.main.async {
                    self.output = finishedText
                    self.failed = !succeeded
                    self.status = succeeded ? "\(action) finished" : "\(action) failed"
                    self.busy = false
                    if succeeded, let path = encodedPath { self.recording = path }
                }
            } catch {
                DispatchQueue.main.async {
                    self.output = error.localizedDescription
                    self.failed = true
                    self.status = "Unable to start the client"
                    self.busy = false
                }
            }
        }
    }
}

struct WorkbenchView: View {
    @StateObject var model = WorkbenchModel()
    var body: some View {
        VStack(alignment: .leading, spacing: 18) {
            HStack {
                VStack(alignment: .leading, spacing: 4) {
                    Text("JTTY Workbench").font(.largeTitle.bold())
                    Text("Encode messages. Decode recordings.").foregroundStyle(.secondary)
                }
                Spacer()
                Text("WAV TOOLS").font(.caption.monospaced()).foregroundStyle(.secondary)
            }
            HStack(spacing: 16) {
                VStack(alignment: .leading) {
                    Text("Lowest tone (Hz)").font(.caption)
                    TextField("1500", text: $model.frequency).frame(width: 110)
                }
                VStack(alignment: .leading) {
                    Text("Receive tolerance (Hz)").font(.caption)
                    TextField("50", text: $model.tolerance).frame(width: 150)
                }
                Spacer()
                Picker("Exchange", selection: $model.profile) {
                    Text("General text").tag("unknown")
                    Text("Field Day").tag("field-day")
                    Text("RTTY Roundup").tag("rtty-roundup")
                }.frame(width: 240)
            }.disabled(model.busy)
            GroupBox("Create a recording") {
                VStack(alignment: .leading, spacing: 10) {
                    TextField("Message (up to 80 characters)", text: $model.message)
                    HStack {
                        Text("12 kHz · mono · 16-bit WAV").font(.caption).foregroundStyle(.secondary)
                        Spacer()
                        Button("Save JTTY WAV…", action: model.encode)
                            .buttonStyle(.borderedProminent).disabled(model.message.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty)
                    }
                }.padding(8)
            }.disabled(model.busy)
            GroupBox("Decode a recording") {
                HStack {
                    TextField("Choose a 12 kHz mono PCM WAV", text: $model.recording)
                    Button("Open…", action: model.chooseRecording)
                    Button("Decode", action: model.decode).disabled(model.recording.isEmpty)
                }.padding(8)
            }.disabled(model.busy)
            HStack {
                if model.busy { ProgressView().controlSize(.small) }
                Text(model.status).foregroundStyle(model.failed ? Color.red : Color.secondary)
                Spacer()
            }.font(.callout)
            ScrollView {
                Text(model.output).font(.system(.body, design: .monospaced))
                    .textSelection(.enabled).frame(maxWidth: .infinity, alignment: .leading).padding(14)
            }.background(Color(nsColor: .textBackgroundColor)).clipShape(RoundedRectangle(cornerRadius: 8))
            Text("Recorded-audio tools · live audio and radio controls are available in Conversation and Station Setup.")
                .font(.caption).foregroundStyle(.secondary)
        }.padding(24).frame(minWidth: 740, minHeight: 590)
    }
}

final class AppDelegate: NSObject, NSApplicationDelegate {
    var window: NSWindow!
    let station = LiveStation()
    func applicationDidFinishLaunching(_ notification: Notification) {
        if let icon = Bundle.main.url(forResource: "JTTY", withExtension: "icns"),
           let image = NSImage(contentsOf: icon) {
            NSApp.applicationIconImage = image
        }
        let menu = NSMenu()
        let appMenu = NSMenu()
        appMenu.addItem(withTitle: "About JTTY Workbench", action: #selector(showAbout), keyEquivalent: "")
        appMenu.addItem(.separator())
        appMenu.addItem(withTitle: "Quit JTTY Workbench", action: #selector(NSApplication.terminate(_:)), keyEquivalent: "q")
        let item = NSMenuItem(); item.submenu = appMenu; menu.addItem(item)
        let edit = NSMenu(title: "Edit")
        for (name, action, key) in [("Cut", "cut:", "x"), ("Copy", "copy:", "c"), ("Paste", "paste:", "v"), ("Select All", "selectAll:", "a")] {
            edit.addItem(withTitle: name, action: Selector(action), keyEquivalent: key)
        }
        let editItem = NSMenuItem(title: "Edit", action: nil, keyEquivalent: ""); editItem.submenu = edit; menu.addItem(editItem)
        NSApp.mainMenu = menu
        window = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 1020, height: 860), styleMask: [.titled, .closable, .miniaturizable, .resizable], backing: .buffered, defer: false)
        window.title = "JTTY Workbench"
        window.contentView = NSHostingView(rootView: StationRootView(station: station))
        window.center()
        window.makeKeyAndOrderFront(nil)
        NSApp.activate(ignoringOtherApps: true)
    }
    @objc func showAbout() {
        let alert = NSAlert()
        alert.messageText = "JTTY Workbench 0.2"
        alert.informativeText = "Independent, unofficial client based on WSJT-X.\n\nJTTY algorithms and source: Copyright © 2001–2026 Joe Taylor, Steven Franke and the WSJT Development Team and contributors. Full upstream attribution appears in THIRD_PARTY_NOTICES.md.\n\nGNU GPL version 3 or later. No warranty. You may redistribute and modify this software under the license."
        alert.addButton(withTitle: "Close")
        alert.addButton(withTitle: "View License")
        if alert.runModal() == .alertSecondButtonReturn {
            let root = Bundle.main.bundleURL.deletingLastPathComponent().deletingLastPathComponent()
            NSWorkspace.shared.open(root.appendingPathComponent("COPYING"))
        }
    }
    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool { true }
    func applicationShouldTerminate(_ sender: NSApplication) -> NSApplication.TerminateReply {
        if let error = station.shutdown() {
            let alert = NSAlert(); alert.messageText = "Radio PTT release failed"; alert.informativeText = error
            alert.addButton(withTitle: "Keep App Open"); alert.addButton(withTitle: "Quit Anyway")
            return alert.runModal() == .alertFirstButtonReturn ? .terminateCancel : .terminateNow
        }
        return .terminateNow
    }
}

@main
enum WorkbenchApp {
    static func main() {
        let application = NSApplication.shared
        let delegate = AppDelegate()
        application.delegate = delegate
        application.setActivationPolicy(.regular)
        withExtendedLifetime(delegate) { application.run() }
    }
}
