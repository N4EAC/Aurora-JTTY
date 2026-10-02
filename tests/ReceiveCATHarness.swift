// No hardware or audio permission required: verify receive lifecycle around real dummy CAT.
import Foundation

final class SyntheticInput: StationAudio {
    var onSamples: (([Int16], Float, [Float]) -> Void)?
    var onError: ((String) -> Void)?
    var running = false, starts = 0, stops = 0, deliveries = 0
    private var timer: Timer?
    func startInput(device: AudioDevice, channel: Int) throws {
        precondition(device.uid == "test-usb" && channel == 0)
        running = true; starts += 1
        timer = Timer.scheduledTimer(withTimeInterval: 0.02, repeats: true) { [weak self] _ in
            guard let self = self, self.running else { return }
            self.deliveries += 1
            let samples = (0..<240).map { Int16(3000 * sin(2 * Double.pi * 1500 * Double($0) / 12000)) }
            self.onSamples?(samples, 3000.0 / 32767, [Float](repeating: 0.5, count: 256))
        }
    }
    func stopInput() { if running { stops += 1 }; running = false; timer?.invalidate(); timer = nil }
    var outputRunning: Bool { false }
    func stopOutput() {}
    func play(samples: [Int16], device: AudioDevice, channel: Int, gain: Float) throws -> Double { fatalError("This receive test must not transmit") }
}

@main enum ReceiveCATHarness {
    static func run(_ seconds: Double) { RunLoop.main.run(until: Date().addingTimeInterval(seconds)) }
    static func main() {
        let input = SyntheticInput()
        let device = AudioDevice(id: 999, uid: "test-usb", name: "Synthetic USB", inputs: 1, outputs: 0, rate: 48000)
        let station = LiveStation(preferences: nil, audio: input, deviceList: { [device] }, permission: { .authorized })
        station.settings.inputUID = device.uid
        station.settings.model = 1
        station.startMonitor(); run(0.2)
        precondition(station.monitoring && input.running && station.peak > 0)
        let before = input.deliveries
        station.connect(); run(2.5) // Includes connection and multiple CAT polls.
        precondition(station.connected && station.monitoring && input.running)
        precondition(input.deliveries > before + 20 && input.starts == 1 && input.stops == 0)
        station.applyFrequency(); run(1.5)
        precondition(station.monitoring && station.radioMode == "PKTUSB" && !station.radioPTT)
        precondition(station.failure.isEmpty, station.failure)
        precondition(station.shutdown() == nil)
        precondition(!input.running)
        print("Receive lifecycle passed: dummy CAT connect, poll and frequency/mode change preserve live samples; no hardware or PTT")
    }
}
