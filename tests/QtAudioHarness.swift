import Foundation

@main enum QtAudioHarness {
    static func main() throws {
        let samples: [Int16] = (0..<12000).map { Int16(12000 * sin(2 * Double.pi * 1500 * Double($0) / 12000)) }
        var raw = Data()
        for sample in samples { let word = UInt16(bitPattern: sample); raw.append(UInt8(word & 255)); raw.append(UInt8(word >> 8)) }
        let file = FileManager.default.temporaryDirectory.appendingPathComponent("jtty-qt-fixture-\(UUID().uuidString).pcm")
        defer { try? FileManager.default.removeItem(at: file) }
        try raw.write(to: file)
        let process = Process(), pipe = Pipe()
        process.executableURL = Bundle.main.executableURL!.deletingLastPathComponent().appendingPathComponent("jtty-audio-input")
        process.arguments = ["--fixture-file", file.path]; process.standardOutput = pipe
        try process.run()
        let output = pipe.fileHandleForReading.readDataToEndOfFile(); process.waitUntilExit()
        precondition(process.terminationStatus == 0 && output == raw, "Upstream AudioDevice pipe altered PCM data")
        let decoder = MonoPCM16PipeDecoder(); var result: [Float] = []
        var offset = 0
        while offset < output.count {
            let size = min(offset % 2 == 0 ? 333 : 1024, output.count - offset)
            result.append(contentsOf: decoder.append(output.subdata(in: offset..<offset + size)))
            offset += size
        }
        precondition(result.count == samples.count)
        for i in samples.indices { precondition(result[i] == Float(samples[i]) / 32768) }
        print("WSJT-X AudioDevice → helper pipe → Swift PCM conversion passed, including fragmented sample boundaries; no audio device accessed")
    }
}
