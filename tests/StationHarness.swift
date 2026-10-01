// Verify the real Swift transmit workflow against Hamlib's dummy backend.
import Foundation

@main
enum StationHarness {
    static func wait(_ seconds: Double, until condition: () -> Bool) {
        let deadline = Date().addingTimeInterval(seconds)
        while !condition() && Date() < deadline { RunLoop.main.run(until: Date().addingTimeInterval(0.02)) }
        precondition(condition(), "Timed out waiting for station state")
    }
    static func main() throws {
        let folder = FileManager.default.temporaryDirectory.appendingPathComponent("jtty-station-\(UUID().uuidString)")
        defer { try? FileManager.default.removeItem(at: folder) }
        let station = LiveStation(preferences: nil, logs: folder)
        guard let device = station.devices.first(where: { $0.name == "MacBook Pro Speakers" && $0.outputs > 0 }) else { fatalError("Built-in speakers unavailable; no radio device will be selected") }
        station.settings.model = 1
        station.settings.outputUID = device.uid
        station.settings.txGain = 0.000001 // Below audibility, built-in speakers only.
        station.connect()
        wait(3) { station.connected && !station.radioMode.isEmpty }
        station.applyFrequency()
        wait(3) { station.radioMode == "PKTUSB" }
        station.settings.theirCall = "K1ABC"
        station.logContact()
        let contact = try String(contentsOf: folder.appendingPathComponent("contacts.adi"), encoding: .utf8)
        precondition(contact.contains("<MODE:4>MFSK") && contact.contains("<APP_JTTYWORKBENCH_MODE:4>JTTY") && contact.contains("<EOR>"), "Contact export failed")
        station.transmitEnabled = true
        station.enqueue("CQ K1ABC CQ")
        station.enqueue("K1ABC")
        wait(12) { station.conversation.filter { $0.direction == "TX" }.count == 2 && !station.transmitting }
        precondition(station.queue.isEmpty && !station.radioPTT, "Transmit queue did not finish in receive")
        precondition(station.failure.isEmpty, station.failure)
        station.tune()
        wait(3) { station.radioPTT }
        station.stop(reason: nil)
        wait(3) { !station.transmitting && !station.radioPTT }
        station.testPTT()
        wait(3) { !station.transmitting && !station.radioPTT }
        precondition(station.failure.isEmpty, station.failure)
        precondition(station.shutdown() == nil, "Shutdown failed")
        print("Swift workflow passed: CAT → queued audio → PTT release → Tune cancellation → PTT test → shutdown")
    }
}
