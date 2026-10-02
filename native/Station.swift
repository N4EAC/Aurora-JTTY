// Live station workflow. GPL-3.0-or-later, 2026-10-01.
import Foundation
import SwiftUI
import AVFoundation

struct RigState { var frequency: Double; var ptt: Bool; var mode: String; var expired: Bool }

final class RigController {
    private let work = DispatchQueue(label: "jtty.hamlib")
    private var handle: UnsafeMutableRawPointer?
    private var timer: DispatchSourceTimer?
    var onState: ((RigState) -> Void)?
    var onError: ((String) -> Void)?
    private func error(_ code: Int32) -> String { String(cString: jw_radio_error(code)) }
    func connect(model: Int32, path: String, pttPath: String, baud: Int32, stopBits: Int32, ptt: Int32, completion: @escaping (String?) -> Void) {
        Diagnostics.record("CAT connect requested model=\(model) port=\(path) baud=\(baud) stopBits=\(stopBits) pttMethod=\(ptt)")
        work.async { [self] in
            if self.handle != nil { DispatchQueue.main.async { completion("Disconnect the existing radio first") }; return }
            var buffer = [CChar](repeating: 0, count: 256)
            let radio = path.withCString { path in pttPath.withCString { jw_radio_open(model, path, $0, baud, stopBits, ptt, &buffer) } }
            guard let radio = radio else { let message = String(cString: buffer); Diagnostics.record("CAT open failed: " + message); DispatchQueue.main.async { completion(message) }; return }
            Diagnostics.record("CAT open succeeded model=\(model) serialControlLines=\(model != 1 && model != 2 ? "DTR/RTS OFF" : "not applicable")")
            self.handle = radio
            var frequency: Double = 0, ptt: Int32 = 0, mode = [CChar](repeating: 0, count: 32)
            let result = jw_radio_state(radio, &frequency, &ptt, &mode)
            if result != 0 {
                _ = jw_radio_close(radio); self.handle = nil
                let message = self.error(result); DispatchQueue.main.async { completion(message) }; return
            }
            let timer = DispatchSource.makeTimerSource(queue: self.work)
            timer.schedule(deadline: .now(), repeating: 1)
            timer.setEventHandler { [weak owner = self] in owner?.poll() }
            self.timer = timer; timer.resume()
            DispatchQueue.main.async { completion(nil) }
        }
    }
    private func poll() {
        guard let handle = handle else { return }
        var frequency: Double = 0, ptt: Int32 = 0, mode = [CChar](repeating: 0, count: 32)
        let result = jw_radio_state(handle, &frequency, &ptt, &mode)
        Diagnostics.record("CAT poll result=\(result) frequency=\(frequency) ptt=\(ptt) mode=\(String(cString: mode))")
        if result == 0 {
            let state = RigState(frequency: frequency, ptt: ptt != 0, mode: String(cString: mode), expired: jw_radio_watchdog(handle) != 0)
            DispatchQueue.main.async { self.onState?(state) }
        } else { let message = error(result); DispatchQueue.main.async { self.onError?("CAT connection lost: \(message)") } }
    }
    func ptt(_ on: Bool, limit: Double = 40, completion: @escaping (String?) -> Void) {
        Diagnostics.record("CAT PTT requested on=\(on) limit=\(limit)")
        work.async {
            let result = jw_radio_ptt(self.handle, on ? 1 : 0, limit)
            let message = result == 0 ? nil : self.error(result)
            DispatchQueue.main.async { completion(message) }
        }
    }
    func setFrequency(_ frequency: Double, dataMode: Bool, completion: @escaping (String?) -> Void) {
        work.async {
            var result = jw_radio_frequency(self.handle, frequency)
            if result == 0 { result = jw_radio_mode(self.handle, dataMode ? 1 : 0) }
            let message = result == 0 ? nil : self.error(result)
            DispatchQueue.main.async { completion(message) }
        }
    }
    func disconnect(completion: @escaping (String?) -> Void) {
        work.async {
            let result = jw_radio_close(self.handle)
            if result == 0 { self.handle = nil; self.timer?.cancel(); self.timer = nil }
            let message = result == 0 ? nil : self.error(result)
            DispatchQueue.main.async { completion(message) }
        }
    }
    func shutdown() -> String? {
        work.sync {
            let result = jw_radio_close(handle)
            if result == 0 { handle = nil; timer?.cancel(); timer = nil; return nil }
            return "PTT release failed: \(error(result)). Check the radio before quitting."
        }
    }
}

struct StationSettings: Codable {
    var inputUID = "", outputUID = ""
    var inputChannel = 0, outputChannel = 0
    var model = 1049, serialPath = "", pttPath = "", baud = 38400, stopBits = 1, pttMethod = 0
    var myCall = "", myGrid = "", theirCall = "", queuedCall = "", serialNumber = 1
    var tone = 1500.0, tolerance = 100.0, dialMHz = 14.090
    var txGain = 0.15, leadMS = 200.0, tailMS = 150.0
    var profile = "unknown", dataMode = true
    var macros = ["CQ %M CQ", "%H %E", "%H TU CQ %M CQ", "%M", "%H", "TU NOW %Q %E", "%H AGN?", "%E"]
}

struct Conversation: Identifiable {
    var id: String
    var date: Date
    var direction: String
    var text: String
    var frequency: Float
    var complete: Bool
}

final class LiveStation: ObservableObject {
    @Published var settings: StationSettings
    @Published var devices: [AudioDevice] = []
    @Published var ports: [String] = []
    @Published var connected = false
    @Published var connecting = false
    @Published var activeInput = ""
    @Published var monitoring = false
    @Published var transmitEnabled = false
    @Published var transmitting = false
    @Published var radioPTT = false
    @Published var dial = "Disconnected"
    @Published var radioMode = ""
    @Published var status = "Select USB audio devices and connect the FT-710."
    @Published var failure = ""
    @Published var peak: Float = 0
    @Published var waterfall: [[Float]] = []
    @Published var conversation: [Conversation] = []
    @Published var draft = ""
    @Published var queue: [String] = []
    private let audio: StationAudio
    private let enumerateDevices: () -> [AudioDevice]
    private let inputPermission: () -> AVAuthorizationStatus
    private let rig = RigController()
    private let dsp = DispatchQueue(label: "jtty.dsp")
    private let backlogLock = NSLock()
    private var backlog = 0
    private var monitorEpoch = UUID().uuidString
    private var txGeneration = 0
    private var monitoringBeforeTX = false
    private var txTimer: Timer?
    private var txDeadline = Date.distantFuture
    private var monitorStarted = Date()
    let logFolder: URL
    private let preferences: UserDefaults?

    init(preferences: UserDefaults? = .standard, logs: URL? = nil,
         audio: StationAudio = RadioAudio(), deviceList: @escaping () -> [AudioDevice] = Devices.list,
         permission: @escaping () -> AVAuthorizationStatus = { AVCaptureDevice.authorizationStatus(for: .audio) }) {
        self.audio = audio; self.enumerateDevices = deviceList; self.inputPermission = permission
        Diagnostics.record("APP start version=\(Bundle.main.infoDictionary?["CFBundleShortVersionString"] ?? "test")")
        self.preferences = preferences
        if let data = preferences?.data(forKey: "station-settings"), let saved = try? JSONDecoder().decode(StationSettings.self, from: data), saved.macros.count == 8 { settings = saved }
        else { settings = StationSettings() }
        logFolder = logs ?? FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0].appendingPathComponent("JTTY Workbench/Logs")
        refresh()
        rig.onState = { [weak self] state in
            guard let self = self else { return }
            self.dial = String(format: "%.6f MHz", state.frequency / 1e6)
            self.radioMode = state.mode; self.radioPTT = state.ptt
            if state.expired && self.transmitting { self.stop(reason: "Transmit watchdog expired") }
        }
        rig.onError = { [weak self] message in
            guard let self = self else { return }
            self.transmitEnabled = false
            if self.transmitting { self.stop(reason: message) }
            self.failure = message
        }
        audio.onError = { [weak self] message in Diagnostics.record("AUDIO error: " + message); DispatchQueue.main.async { self?.stop(reason: message); self?.stopMonitor() } }
    }

    func save() {
        if let data = try? JSONEncoder().encode(settings) { preferences?.set(data, forKey: "station-settings") }
    }
    func refresh() {
        devices = enumerateDevices()
        Diagnostics.record("DEVICES " + devices.map { "\($0.name) id=\($0.id) uid=\($0.uid) in=\($0.inputs) out=\($0.outputs) rate=\($0.rate)" }.joined(separator: " | "))
        ports = ((try? FileManager.default.contentsOfDirectory(atPath: "/dev")) ?? []).filter { $0.hasPrefix("cu.") && !$0.contains("Bluetooth") && !$0.contains("debug-console") }.map { "/dev/" + $0 }
    }
    func connect() {
        guard !connected, !connecting else { return }
        if settings.model != 1 && settings.serialPath.isEmpty { failure = "Choose the radio's Enhanced USB serial port or enter a rigctld address."; return }
        if settings.model != 1 && settings.model != 2 && (settings.pttMethod == 1 || settings.pttMethod == 2) && settings.pttPath.isEmpty { failure = "Select a separate Standard USB port for RTS/DTR PTT"; return }
        save(); connecting = true; failure = ""
        rig.connect(model: Int32(settings.model), path: settings.serialPath, pttPath: settings.pttPath, baud: Int32(settings.baud), stopBits: Int32(settings.stopBits), ptt: Int32(settings.pttMethod)) { error in
            self.connecting = false
            if let error = error { self.failure = "Cannot connect: \(error)" }
            else { self.connected = true; self.status = self.settings.model == 1 ? "Connected to Hamlib dummy radio" : "CAT connected. Radio settings were read." }
        }
    }
    func disconnect() {
        transmitEnabled = false; stop(reason: nil); stopMonitor()
        rig.disconnect { error in
            if let error = error { self.failure = "PTT release failed; connection kept open: \(error)" }
            else { self.connected = false; self.radioPTT = false; self.dial = "Disconnected"; self.status = "Disconnected" }
        }
    }
    func applyFrequency() {
        guard connected, !transmitting, !radioPTT else { return }
        let frequency = settings.dialMHz * 1e6
        guard frequency.isFinite, frequency >= 100000, frequency <= 60000000 else { failure = "Enter a valid dial frequency in MHz"; return }
        rig.setFrequency(frequency, dataMode: settings.dataMode) { error in
            if let error = error { self.failure = "Frequency/mode change failed: \(error)" }
            else { self.status = "Frequency and upper-sideband mode applied"; self.save() }
        }
    }
    func startMonitor() {
        guard !monitoring, !transmitting else { return }
        Diagnostics.record("MONITOR start requested uid=\(settings.inputUID) channel=\(settings.inputChannel) CATconnected=\(connected)")
        refresh()
        guard let device = devices.first(where: { $0.uid == settings.inputUID && $0.inputs > 0 }) else { failure = "Select an audio input device"; return }
        guard validAudioSettings() else { return }
        let permission = inputPermission()
        if permission == .notDetermined {
            AVCaptureDevice.requestAccess(for: .audio) { allowed in DispatchQueue.main.async {
                if allowed { self.startMonitor() } else { self.failure = "Allow audio input for JTTY Workbench in macOS Privacy & Security." }
            } }; return
        }
        if permission != .authorized { failure = "Audio input permission is required in macOS Privacy & Security."; return }
        dsp.sync { jw_reset() }
        monitorEpoch = UUID().uuidString; monitorStarted = Date()
        let epoch = monitorEpoch
        audio.onSamples = { [weak self] samples, peak, spectrum in self?.receive(samples, peak: peak, spectrum: spectrum, epoch: epoch) }
        do { try audio.startInput(device: device, channel: settings.inputChannel); activeInput = "\(device.name) · channel \(settings.inputChannel + 1)"; monitoring = true; failure = ""; status = "Receiving from \(device.name)"; save() }
        catch { failure = error.localizedDescription }
    }
    func stopMonitor() { Diagnostics.record("MONITOR stop CATconnected=\(connected) transmitting=\(transmitting)"); monitoring = false; activeInput = ""; monitorEpoch = UUID().uuidString; audio.stopInput(); peak = 0 }
    func validAudioSettings() -> Bool {
        guard settings.tone.isFinite, (200...2600).contains(settings.tone), settings.tolerance.isFinite, (1...1000).contains(settings.tolerance), settings.txGain.isFinite, (0...1).contains(settings.txGain), (0...2000).contains(settings.leadMS), (0...2000).contains(settings.tailMS) else { failure = "Check audio frequency, tolerance, gain and PTT timing values."; return false }
        return true
    }
    private func receive(_ samples: [Int16], peak: Float, spectrum: [Float], epoch: String) {
        backlogLock.lock()
        if backlog > 24 { backlogLock.unlock(); DispatchQueue.main.async { self.stopMonitor(); self.failure = "Decoder cannot keep up. Monitoring stopped; restart after reducing load." }; return }
        backlog += 1; backlogLock.unlock()
        // Capture mutable UI settings on the main queue before serial DSP use.
        DispatchQueue.main.async {
            guard self.monitoring, epoch == self.monitorEpoch else {
                self.backlogLock.lock(); self.backlog -= 1; self.backlogLock.unlock(); return
            }
            self.peak = peak
            if !spectrum.isEmpty { self.waterfall.append(spectrum); if self.waterfall.count > 90 { self.waterfall.removeFirst() } }
            let tone = Float(self.settings.tone), tolerance = Float(self.settings.tolerance), base = self.monitorStarted
            self.dsp.async {
                defer { self.backlogLock.lock(); self.backlog -= 1; self.backlogLock.unlock() }
                _ = samples.withUnsafeBufferPointer { jw_feed($0.baseAddress, Int32(samples.count), tone, tolerance) }
                while true {
                    var id: Int64 = 0, hz: Float = 0, seconds: Double = 0, complete: Int32 = 0
                    var text = [CChar](repeating: 0, count: 81)
                    if jw_pop(&id, &hz, &seconds, &complete, &text) == 0 { break }
                    let update = Conversation(id: "\(epoch)-\(id)", date: base.addingTimeInterval(seconds), direction: "RX", text: String(cString: text).trimmingCharacters(in: .whitespaces), frequency: hz, complete: complete != 0)
                    DispatchQueue.main.async { if epoch == self.monitorEpoch { self.add(update) } }
                }
            }
        }
    }
    private func add(_ item: Conversation) {
        if let index = conversation.firstIndex(where: { $0.id == item.id }) { conversation[index] = item }
        else { conversation.append(item) }
        if conversation.count > 500 { conversation.removeFirst() }
        if item.complete { log(item) }
    }
    private func log(_ item: Conversation) {
        do {
            try FileManager.default.createDirectory(at: logFolder, withIntermediateDirectories: true)
            let formatter = DateFormatter(); formatter.dateFormat = "yyyy-MM-dd"; formatter.timeZone = TimeZone(secondsFromGMT: 0)
            let url = logFolder.appendingPathComponent(formatter.string(from: item.date) + ".jsonl")
            let record: [String: Any] = ["id": item.id, "timestamp": ISO8601DateFormatter().string(from: item.date), "direction": item.direction, "text": item.text, "audio_hz": item.frequency, "dial": dial]
            var data = try JSONSerialization.data(withJSONObject: record); data.append(10)
            if !FileManager.default.fileExists(atPath: url.path) { FileManager.default.createFile(atPath: url.path, contents: nil) }
            let file = try FileHandle(forWritingTo: url); defer { try? file.close() }
            try file.seekToEnd(); try file.write(contentsOf: data)
        } catch { failure = "Conversation log failed: \(error.localizedDescription)" }
    }
    func enqueue(_ message: String? = nil) {
        let text = (message ?? draft).uppercased().split(whereSeparator: { $0.isWhitespace }).joined(separator: " ")
        let alphabet = Set("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ +-./?!\"#$%,&*()_'=[]{}<>|:;")
        guard !text.isEmpty, text.count <= 80, text.allSatisfy({ alphabet.contains($0) }) else { failure = "Use 1–80 supported letters, numbers or punctuation characters."; return }
        guard connected, transmitEnabled, settings.pttMethod != 3 else { failure = "Connect the radio and enable transmission first."; return }
        guard queue.count < 20 else { failure = "Transmit queue is full"; return }
        queue.append(text); if message == nil { draft = "" }
        if !transmitting { sendNext() }
    }
    func macro(_ index: Int) {
        guard settings.macros.indices.contains(index) else { return }
        let text = settings.macros[index]
        if text.contains("%M") && settings.myCall.isEmpty { failure = "Enter your callsign first"; return }
        if text.contains("%H") && settings.theirCall.isEmpty { failure = "Enter the other station's callsign first"; return }
        if text.contains("%Q") && settings.queuedCall.isEmpty { failure = "Enter a queued callsign first"; return }
        enqueue(text.replacingOccurrences(of: "%M", with: settings.myCall).replacingOccurrences(of: "%H", with: settings.theirCall).replacingOccurrences(of: "%Q", with: settings.queuedCall).replacingOccurrences(of: "%E", with: String(format: "599 %03d", settings.serialNumber)))
    }
    func tune() {
        guard connected, transmitEnabled, !transmitting, !radioPTT, settings.pttMethod != 3 else { failure = "Connect the radio and enable transmission before Tune"; return }
        queue = ["TUNE"]
        sendNext(tune: true)
    }
    func testPTT() {
        guard connected, transmitEnabled, !transmitting, !radioPTT, settings.pttMethod != 3 else { return }
        txGeneration += 1; let generation = txGeneration
        transmitting = true; status = "Testing PTT for one second"
        rig.ptt(true, limit: 3) { error in
            guard generation == self.txGeneration else { return }
            if let error = error { self.stop(reason: "PTT test failed: \(error)"); return }
            DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
                guard generation == self.txGeneration else { return }
                self.stop(reason: nil)
            }
        }
    }
    private func sendNext(tune: Bool = false) {
        guard !queue.isEmpty, connected, transmitEnabled else { return }
        guard validAudioSettings(), let device = devices.first(where: { $0.uid == settings.outputUID && $0.outputs > settings.outputChannel }), settings.txGain > 0 else { failure = "Select an output device/channel and a nonzero transmit level"; queue.removeAll(); return }
        guard !radioPTT else { failure = "Radio is already transmitting; wait for receive before sending"; queue.removeAll(); return }
        guard radioMode == "USB" || radioMode == "PKTUSB" else { failure = "Select USB or Data USB using Apply Frequency / Mode before sending"; queue.removeAll(); return }
        let text = queue.removeFirst(), tone = Float(settings.tone), profile: Int32 = settings.profile == "rtty-roundup" ? 2 : settings.profile == "field-day" ? 1 : 0
        transmitting = true; failure = ""; status = "Preparing JTTY message"
        txGeneration += 1; let generation = txGeneration
        monitoringBeforeTX = monitoringBeforeTX || monitoring; stopMonitor()
        let gain = Float(settings.txGain), channel = settings.outputChannel, lead = settings.leadMS / 1000, tail = settings.tailMS / 1000
        dsp.async {
            var pcm = [Int16](repeating: 0, count: 400000), normalized = [CChar](repeating: 0, count: 81)
            let count: Int32
            if tune {
                count = 36000
                for i in 0..<Int(count) {
                    let ramp = min(1, min(Double(i) / 200, Double(Int(count) - 1 - i) / 200))
                    pcm[i] = Int16(sin(2 * Double.pi * Double(tone) * Double(i) / 12000) * 0.7 * 32767 * ramp)
                }
            } else { count = text.withCString { jw_encode($0, profile, tone, &pcm, Int32(pcm.count), &normalized) } }
            let samples = count > 0 ? Array(pcm.prefix(Int(count))) : []
            let rendered = tune ? "Tune (3 seconds)" : String(cString: normalized).trimmingCharacters(in: .whitespaces)
            DispatchQueue.main.async {
                guard generation == self.txGeneration else { return }
                if count <= 0 { self.stop(reason: "Message could not be encoded"); return }
                self.status = "Keying radio"
                self.rig.ptt(true, limit: min(45, Double(count) / 12000 + lead + tail + 5)) { error in
                    guard generation == self.txGeneration else { self.rig.ptt(false) { _ in }; return }
                    if let error = error { self.stop(reason: "PTT failed: \(error)"); return }
                    DispatchQueue.main.asyncAfter(deadline: .now() + lead) {
                        guard generation == self.txGeneration else { return }
                        do {
                            let duration = try self.audio.play(samples: samples, device: device, channel: channel, gain: gain)
                            self.status = "Transmitting \(rendered)"
                            let started = Date(); self.txDeadline = started.addingTimeInterval(duration + 3)
                            self.txTimer = Timer.scheduledTimer(withTimeInterval: 0.05, repeats: true) { _ in
                                if generation != self.txGeneration { return }
                                if Date() > self.txDeadline { self.stop(reason: "Audio output did not finish in time"); return }
                                if Date().timeIntervalSince(started) > 0.1 && !self.audio.outputRunning {
                                    self.txTimer?.invalidate(); self.txTimer = nil
                                    DispatchQueue.main.asyncAfter(deadline: .now() + tail) {
                                        guard generation == self.txGeneration else { return }
                                        self.rig.ptt(false) { error in
                                            guard generation == self.txGeneration else { return }
                                            if let error = error { self.stop(reason: "PTT release failed: \(error)"); return }
                                            self.audio.stopOutput(); self.transmitting = false; self.radioPTT = false
                                            self.add(Conversation(id: UUID().uuidString, date: started, direction: "TX", text: rendered, frequency: tone, complete: true))
                                            self.status = "Message sent"
                                            if !self.queue.isEmpty { self.sendNext() }
                                            else if self.monitoringBeforeTX { self.startMonitor() }
                                        }
                                    }
                                }
                            }
                        } catch { self.stop(reason: error.localizedDescription) }
                    }
                }
            }
        }
    }
    func stop(reason: String?) {
        let resume = monitoringBeforeTX && transmitting
        txGeneration += 1; txTimer?.invalidate(); txTimer = nil; queue.removeAll(); audio.stopOutput()
        let generation = txGeneration
        transmitting = connected; monitoringBeforeTX = false
        if let reason = reason { failure = reason; transmitEnabled = false }
        status = connected ? "Releasing PTT" : "Stopped"
        if connected {
            rig.ptt(false) { error in
                guard generation == self.txGeneration else { return }
                self.transmitting = false
                if let error = error { self.failure = "PTT RELEASE FAILED: \(error). Check the radio."; self.transmitEnabled = false }
                else { self.radioPTT = false; self.status = "Stopped"; if resume { self.startMonitor() } }
            }
        }
    }
    func shutdown() -> String? {
        txGeneration += 1; txTimer?.invalidate(); audio.stopOutput(); stopMonitor(); save()
        return rig.shutdown()
    }
    func logContact() {
        let call = settings.theirCall.uppercased().trimmingCharacters(in: .whitespacesAndNewlines)
        guard connected, !call.isEmpty, call.allSatisfy({ $0.isASCII && ($0.isLetter || $0.isNumber || $0 == "/") }) else { failure = "Enter a valid contact callsign"; return }
        let date = Date(), formatter = DateFormatter(); formatter.timeZone = TimeZone(secondsFromGMT: 0)
        formatter.dateFormat = "yyyyMMdd"; let day = formatter.string(from: date)
        formatter.dateFormat = "HHmmss"; let time = formatter.string(from: date)
        let fields = [("CALL",call),("QSO_DATE",day),("TIME_ON",time),("MODE","MFSK"),("APP_JTTYWORKBENCH_MODE","JTTY"),("COMMENT","JTTY contact"),("FREQ",dial.replacingOccurrences(of: " MHz", with: "")),("STATION_CALLSIGN",settings.myCall.uppercased()),("MY_GRIDSQUARE",settings.myGrid.uppercased())]
        let line = fields.filter { !$0.1.isEmpty }.map { "<\($0.0):\($0.1.utf8.count)>\($0.1)" }.joined(separator: " ") + " <EOR>\n"
        do {
            try FileManager.default.createDirectory(at: logFolder, withIntermediateDirectories: true)
            let url = logFolder.appendingPathComponent("contacts.adi")
            if !FileManager.default.fileExists(atPath: url.path) { try "JTTY Workbench <ADIF_VER:5>3.1.7 <EOH>\n".write(to: url, atomically: true, encoding: .utf8) }
            let file = try FileHandle(forWritingTo: url); defer { try? file.close() }; try file.seekToEnd(); try file.write(contentsOf: Data(line.utf8))
            settings.serialNumber += 1; save(); status = "Logged contact with \(call)"
        } catch { failure = "Contact log failed: \(error.localizedDescription)" }
    }
}
