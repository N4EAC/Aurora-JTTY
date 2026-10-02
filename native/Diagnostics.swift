// Local development diagnostics. No audio samples or message contents are logged.
import Foundation

enum Diagnostics {
    static let lock = NSLock()
    static let url: URL = {
        if Bundle.main.bundleURL.pathExtension == "app" {
            return FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0].appendingPathComponent("JTTY Workbench/Logs/diagnostics.log")
        }
        return FileManager.default.temporaryDirectory.appendingPathComponent("jtty-test-diagnostics.log")
    }()
    static func record(_ event: String) {
        lock.lock(); defer { lock.unlock() }
        do {
            try FileManager.default.createDirectory(at: url.deletingLastPathComponent(), withIntermediateDirectories: true)
            if let size = try? url.resourceValues(forKeys: [.fileSizeKey]).fileSize, size > 2_000_000 {
                let previous = url.appendingPathExtension("previous")
                try? FileManager.default.removeItem(at: previous)
                try FileManager.default.moveItem(at: url, to: previous)
            }
            if !FileManager.default.fileExists(atPath: url.path) { FileManager.default.createFile(atPath: url.path, contents: nil) }
            let handle = try FileHandle(forWritingTo: url); defer { try? handle.close() }
            try handle.seekToEnd()
            let formatter = ISO8601DateFormatter()
            formatter.formatOptions = [.withInternetDateTime, .withFractionalSeconds]
            let line = formatter.string(from: Date()) + " pid=\(ProcessInfo.processInfo.processIdentifier) " + event.replacingOccurrences(of: "\n", with: " ") + "\n"
            try handle.write(contentsOf: Data(line.utf8))
        } catch { NSLog("JTTY diagnostic write failed: %@", error.localizedDescription) }
    }
}
