// Exercise native channel selection, sample-rate conversion and silent output.
import Foundation
import AVFoundation
import CoreAudio
import Darwin

@main
enum AudioHarness {
    static func main() throws {
        setbuf(stdout, nil)
        if CommandLine.arguments.contains("--list") {
            for device in Devices.list() { print("\(device.id)\t\(device.name)\tinput=\(device.inputs) output=\(device.outputs) rate=\(device.rate)") }
            return
        }
        if CommandLine.arguments.contains("--diagnose-usb") {
            if #available(macOS 14.0, *) { print("Application input muted: \(AVAudioApplication.shared.isInputMuted)") }
            for device in Devices.list().filter({ $0.name.localizedCaseInsensitiveContains("USB") }) {
                print("USB id=\(device.id) uid=\(device.uid)")
                for (name, selector) in [("alive", kAudioDevicePropertyDeviceIsAlive), ("running", kAudioDevicePropertyDeviceIsRunningSomewhere), ("hogPID", kAudioDevicePropertyHogMode)] {
                    var address = AudioObjectPropertyAddress(mSelector: selector, mScope: kAudioObjectPropertyScopeGlobal, mElement: kAudioObjectPropertyElementMain)
                    var value: Int32 = -999, bytes: UInt32 = 4
                    let result = AudioObjectGetPropertyData(device.id, &address, 0, nil, &bytes, &value)
                    print("\(name)=\(value), status=\(result)")
                }
            }
            return
        }
        if CommandLine.arguments.contains("--usb-capture") {
            guard AVCaptureDevice.authorizationStatus(for: .audio) == .authorized else {
                print("Audio input permission not granted to this test executable; test skipped")
                return
            }
            guard let device = Devices.list().first(where: { $0.name.localizedCaseInsensitiveContains("USB") && $0.inputs > 0 }) else {
                print("No USB audio input present; test skipped")
                return
            }
            var radio: UnsafeMutableRawPointer?
            if CommandLine.arguments.contains("--dummy-cat") {
                var error = [CChar](repeating: 0, count: 256)
                radio = jw_radio_open(1, "", "", 38400, 1, 0, &error)
                precondition(radio != nil, String(cString: error))
                var frequency: Double = 0, ptt: Int32 = 0, mode = [CChar](repeating: 0, count: 32)
                precondition(jw_radio_state(radio, &frequency, &ptt, &mode) == 0)
                print("Dummy CAT connected and queried; no PTT commands")
            }
            defer { if let radio = radio { _ = jw_radio_close(radio) } }
            let audio = RadioAudio(), lock = NSLock()
            var batches = 0, peak: Float = 0, failures: [String] = []
            audio.onSamples = { _, level, _ in lock.lock(); batches += 1; peak = max(peak, level); lock.unlock() }
            audio.onError = { message in lock.lock(); failures.append(message); lock.unlock() }
            for channel in 0..<device.inputs {
                for _ in 0..<3 {
                    try audio.startInput(device: device, channel: channel)
                    RunLoop.main.run(until: Date().addingTimeInterval(3.0))
                    audio.stopInput()
                }
            }
            lock.lock(); let received = batches, maximum = peak, errors = failures; lock.unlock()
            print("Capture result: batches=\(received), peak=\(maximum), errors=\(errors)")
            precondition(received > 0, "No capture callbacks received")
            precondition(errors.isEmpty, errors.joined(separator: "; "))
            if maximum == 0 { print("WARNING: callbacks work, but USB samples are all zero; signal reception is NOT verified") }
            print("USB callback/restart check completed on \(device.name): \(received) batches, peak \(maximum), \(device.inputs) channels; no playback or PTT")
            return
        }
        if CommandLine.arguments.contains("--silent-output") {
            guard let device = Devices.list().first(where: { $0.name == "MacBook Pro Speakers" && $0.outputs > 0 }) else { fatalError("Built-in speakers unavailable; no automatic routing to a radio device") }
            let audio = RadioAudio()
            _ = try audio.play(samples: [Int16](repeating: 0, count: 3600), device: device, channel: 0, gain: 0)
            var observedRunning = false
            let deadline = Date().addingTimeInterval(3)
            while Date() < deadline {
                observedRunning = observedRunning || audio.outputRunning
                if observedRunning && !audio.outputRunning { break }
                Thread.sleep(forTimeInterval: 0.02)
            }
            precondition(observedRunning && !audio.outputRunning, "Output failed to drain")
            audio.stopOutput()
            print("Silent output drained successfully on \(device.name)")
            return
        }
        jw_reset()
        var samples = [Int16](repeating: 0, count: 400000), text = [CChar](repeating: 0, count: 81)
        let count = "CQ K1ABC CQ".withCString { jw_encode($0, 0, 1500, &samples, Int32(samples.count), &text) }
        precondition(count > 0)
        // Channel one is a distractor, channel two contains the JTTY waveform at 48 kHz.
        let pcm = [Int16](repeating: 0, count: 6000) + Array(samples.prefix(Int(count))) + [Int16](repeating: 0, count: 36000)
        var interleaved = [Float]()
        for value in pcm {
            for _ in 0..<4 { interleaved.append(0.05); interleaved.append(Float(value) / 32767) }
        }
        let audio = RadioAudio()
        var complete = [String](), receivedCount = 0
        audio.onError = { fatalError($0) }
        audio.onSamples = { samples, _, _ in
            receivedCount += samples.count
            _ = samples.withUnsafeBufferPointer { jw_feed($0.baseAddress, Int32(samples.count), 1500, 100) }
            while true {
                var id: Int64 = 0, hz: Float = 0, seconds: Double = 0, eom: Int32 = 0, message = [CChar](repeating: 0, count: 81)
                if jw_pop(&id, &hz, &seconds, &eom, &message) == 0 { break }
                if eom != 0 { complete.append(String(cString: message).trimmingCharacters(in: .whitespaces)) }
            }
        }
        for offset in stride(from: 0, to: interleaved.count, by: 4096) {
            audio.testCapture(Array(interleaved[offset..<min(offset + 4096, interleaved.count)]), rate: 48000, channels: 2, channel: 1)
        }
        precondition(complete == ["CQ K1ABC CQ"], "Native resampler failed: \(complete)")
        precondition(abs(receivedCount - pcm.count) < 128, "Sample conversion drift")
        print("Native 48 kHz stereo → selected channel → 12 kHz streaming JTTY decode passed")
    }
}
