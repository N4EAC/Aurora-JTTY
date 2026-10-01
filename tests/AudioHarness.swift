// Exercise native channel selection, sample-rate conversion and silent output.
import Foundation

@main
enum AudioHarness {
    static func main() throws {
        if CommandLine.arguments.contains("--list") {
            for device in Devices.list() { print("\(device.id)\t\(device.name)\tinput=\(device.inputs) output=\(device.outputs) rate=\(device.rate)") }
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
