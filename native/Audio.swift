// CoreAudio device routing and live PCM. GPL-3.0-or-later, 2026-10-01.
import Foundation
import CoreAudio
import AudioToolbox
import AVFoundation
import Accelerate

struct AudioDevice: Identifiable, Hashable {
    let id: AudioDeviceID
    let uid: String
    let name: String
    let inputs: Int
    let outputs: Int
    let rate: Double
}

enum AudioFailure: LocalizedError {
    case status(String, OSStatus)
    case message(String)
    var errorDescription: String? {
        switch self {
        case let .status(action, code): return "\(action) failed (CoreAudio \(code))"
        case let .message(message): return message
        }
    }
}

func audioCheck(_ code: OSStatus, _ action: String) throws {
    if code != noErr { throw AudioFailure.status(action, code) }
}

enum Devices {
    static func list() -> [AudioDevice] {
        var address = AudioObjectPropertyAddress(mSelector: kAudioHardwarePropertyDevices, mScope: kAudioObjectPropertyScopeGlobal, mElement: kAudioObjectPropertyElementMain)
        var size: UInt32 = 0
        guard AudioObjectGetPropertyDataSize(AudioObjectID(kAudioObjectSystemObject), &address, 0, nil, &size) == noErr else { return [] }
        var ids = [AudioDeviceID](repeating: 0, count: Int(size) / MemoryLayout<AudioDeviceID>.size)
        guard AudioObjectGetPropertyData(AudioObjectID(kAudioObjectSystemObject), &address, 0, nil, &size, &ids) == noErr else { return [] }
        func string(_ id: AudioDeviceID, _ selector: AudioObjectPropertySelector) -> String {
            var a = AudioObjectPropertyAddress(mSelector: selector, mScope: kAudioObjectPropertyScopeGlobal, mElement: kAudioObjectPropertyElementMain)
            var value: Unmanaged<CFString>?
            var bytes = UInt32(MemoryLayout<UnsafeRawPointer>.size)
            guard AudioObjectGetPropertyData(id, &a, 0, nil, &bytes, &value) == noErr, let value = value else { return "" }
            return value.takeUnretainedValue() as String
        }
        func channels(_ id: AudioDeviceID, _ scope: AudioObjectPropertyScope) -> Int {
            var a = AudioObjectPropertyAddress(mSelector: kAudioDevicePropertyStreamConfiguration, mScope: scope, mElement: kAudioObjectPropertyElementMain)
            var bytes: UInt32 = 0
            guard AudioObjectGetPropertyDataSize(id, &a, 0, nil, &bytes) == noErr, bytes > 0 else { return 0 }
            let memory = UnsafeMutableRawPointer.allocate(byteCount: Int(bytes), alignment: MemoryLayout<AudioBufferList>.alignment)
            defer { memory.deallocate() }
            guard AudioObjectGetPropertyData(id, &a, 0, nil, &bytes, memory) == noErr else { return 0 }
            return UnsafeMutableAudioBufferListPointer(memory.assumingMemoryBound(to: AudioBufferList.self)).reduce(0) { $0 + Int($1.mNumberChannels) }
        }
        return ids.map { id in
            var a = AudioObjectPropertyAddress(mSelector: kAudioDevicePropertyNominalSampleRate, mScope: kAudioObjectPropertyScopeGlobal, mElement: kAudioObjectPropertyElementMain)
            var rate: Double = 48000, bytes = UInt32(MemoryLayout<Double>.size)
            _ = AudioObjectGetPropertyData(id, &a, 0, nil, &bytes, &rate)
            return AudioDevice(id: id, uid: string(id, kAudioDevicePropertyDeviceUID), name: string(id, kAudioObjectPropertyName), inputs: channels(id, kAudioDevicePropertyScopeInput), outputs: channels(id, kAudioDevicePropertyScopeOutput), rate: rate)
        }.sorted { $0.name < $1.name }
    }
}

final class RadioAudio {
    private var input: UnsafeMutableRawPointer?
    private var output: AudioQueueRef?
    private var converter: AVAudioConverter?
    private var sourceFormat: AVAudioFormat?
    private var inputChannels = 1
    private var selectedChannel = 0
    private let processing = DispatchQueue(label: "jtty.audio.capture")
    var onSamples: (([Int16], Float, [Float]) -> Void)?
    var onError: ((String) -> Void)?

    func startInput(device: AudioDevice, channel: Int) throws {
        stopInput()
        guard channel >= 0, channel < device.inputs else { throw AudioFailure.message("Select an available input channel") }
        inputChannels = device.inputs
        selectedChannel = channel
        sourceFormat = AVAudioFormat(commonFormat: .pcmFormatFloat32, sampleRate: device.rate, channels: 1, interleaved: false)
        let target = AVAudioFormat(commonFormat: .pcmFormatFloat32, sampleRate: 12000, channels: 1, interleaved: false)!
        converter = AVAudioConverter(from: sourceFormat!, to: target)
        guard converter != nil else { throw AudioFailure.message("Cannot convert this audio device to 12 kHz") }
        var error = [CChar](repeating: 0, count: 512)
        input = device.name.withCString { name in
            jw_capture_open(device.id, name, Int32(inputChannels), device.rate, { samples, count, context in
                guard let samples = samples, let context = context else { return }
                let owner = Unmanaged<RadioAudio>.fromOpaque(context).takeUnretainedValue()
                // PortAudio owns the callback buffer; copy before returning to its thread.
                let copy = Array(UnsafeBufferPointer(start: samples, count: Int(count)))
                owner.processing.async { owner.consume(copy) }
            }, Unmanaged.passUnretained(self).toOpaque(), &error)
        }
        guard input != nil else {
            stopInput()
            throw AudioFailure.message(String(cString: error))
        }
    }

    private func consume(_ interleaved: [Float]) {
        guard let converter = converter, let format = sourceFormat else { return }
        let frames = interleaved.count / inputChannels
        guard frames > 0, let source = AVAudioPCMBuffer(pcmFormat: format, frameCapacity: AVAudioFrameCount(frames)) else { return }
        source.frameLength = AVAudioFrameCount(frames)
        for i in 0..<frames { source.floatChannelData![0][i] = interleaved[i * inputChannels + selectedChannel] }
        let capacity = AVAudioFrameCount(ceil(Double(frames) * 12000 / format.sampleRate) + 64)
        let destination = AVAudioPCMBuffer(pcmFormat: converter.outputFormat, frameCapacity: capacity)!
        var supplied = false
        var error: NSError?
        let result = converter.convert(to: destination, error: &error) { _, status in
            if supplied { status.pointee = .noDataNow; return nil }
            supplied = true; status.pointee = .haveData; return source
        }
        if result == .error { onError?(error?.localizedDescription ?? "Audio conversion failed"); return }
        let count = Int(destination.frameLength)
        if count == 0 { return }
        let values = Array(UnsafeBufferPointer(start: destination.floatChannelData![0], count: count))
        let peak = values.map { abs($0) }.max() ?? 0
        let pcm = values.map { Int16(max(-32767, min(32767, Int(($0.isFinite ? $0 : 0) * 32767)))) }
        onSamples?(pcm, peak, spectrum(values))
    }

    private func spectrum(_ samples: [Float]) -> [Float] {
        let n = 1024
        var real = [Float](repeating: 0, count: n), imaginary = real, outReal = real, outImaginary = real
        let tail = samples.suffix(n)
        for (i, value) in tail.enumerated() { real[i] = value * (0.5 - 0.5 * cos(2 * Float.pi * Float(i) / Float(n - 1))) }
        guard let setup = vDSP_DFT_zop_CreateSetup(nil, vDSP_Length(n), .FORWARD) else { return [] }
        defer { vDSP_DFT_DestroySetup(setup) }
        vDSP_DFT_Execute(setup, &real, &imaginary, &outReal, &outImaginary)
        return (0..<256).map { index in
            let bin = min(n / 2 - 1, Int((200 + Double(index) * 2600 / 255) * Double(n) / 12000))
            let power = outReal[bin] * outReal[bin] + outImaginary[bin] * outImaginary[bin]
            return max(0, min(1, (10 * log10(max(1e-12, power / Float(n * n))) + 90) / 70))
        }
    }

    func play(samples: [Int16], device: AudioDevice, channel: Int, gain: Float) throws -> Double {
        stopOutput()
        guard channel >= 0, channel < device.outputs else { throw AudioFailure.message("Select an available output channel") }
        let channels = device.outputs
        // AudioQueue converts 12 kHz client PCM into the device's hardware format.
        var format = AudioStreamBasicDescription(mSampleRate: 12000, mFormatID: kAudioFormatLinearPCM,
            mFormatFlags: kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked, mBytesPerPacket: UInt32(4 * channels),
            mFramesPerPacket: 1, mBytesPerFrame: UInt32(4 * channels), mChannelsPerFrame: UInt32(channels), mBitsPerChannel: 32, mReserved: 0)
        var queue: AudioQueueRef?
        try audioCheck(AudioQueueNewOutput(&format, { _, _, _ in }, nil, nil, nil, 0, &queue), "Create output")
        output = queue
        do {
            let string = device.uid as CFString
            var uid = Unmanaged.passUnretained(string).toOpaque()
            try audioCheck(AudioQueueSetProperty(queue!, kAudioQueueProperty_CurrentDevice, &uid, UInt32(MemoryLayout<UnsafeRawPointer>.size)), "Select output device")
            var buffer: AudioQueueBufferRef?
            let size = UInt32(samples.count * channels * 4)
            try audioCheck(AudioQueueAllocateBuffer(queue!, size, &buffer), "Allocate transmit audio")
            let data = buffer!.pointee.mAudioData.assumingMemoryBound(to: Float.self)
            data.initialize(repeating: 0, count: samples.count * channels)
            for i in samples.indices { data[i * channels + channel] = Float(samples[i]) / 32767 * gain }
            buffer!.pointee.mAudioDataByteSize = size
            try audioCheck(AudioQueueEnqueueBuffer(queue!, buffer!, 0, nil), "Queue transmit audio")
            try audioCheck(AudioQueueStart(queue!, nil), "Start transmit audio")
            try audioCheck(AudioQueueStop(queue!, false), "Drain transmit audio")
            return Double(samples.count) / 12000
        } catch { stopOutput(); throw error }
    }

    var outputRunning: Bool {
        guard let output = output else { return false }
        var running: UInt32 = 0, size = UInt32(MemoryLayout<UInt32>.size)
        return AudioQueueGetProperty(output, kAudioQueueProperty_IsRunning, &running, &size) == noErr && running != 0
    }
    func stopOutput() {
        if let queue = output { AudioQueueStop(queue, true); AudioQueueDispose(queue, true); output = nil }
    }
    func stopInput() {
        let capture = input
        input = nil
        // PortAudio waits for callbacks before releasing the capture context.
        if let capture = capture { jw_capture_close(capture) }
        processing.sync { converter = nil; sourceFormat = nil }
    }
    // Exercise the exact channel-selection and streaming conversion path without microphone access.
    func testCapture(_ samples: [Float], rate: Double, channels: Int, channel: Int) {
        if converter == nil {
            inputChannels = channels; selectedChannel = channel
            sourceFormat = AVAudioFormat(commonFormat: .pcmFormatFloat32, sampleRate: rate, channels: 1, interleaved: false)
            let target = AVAudioFormat(commonFormat: .pcmFormatFloat32, sampleRate: 12000, channels: 1, interleaved: false)!
            converter = AVAudioConverter(from: sourceFormat!, to: target)
        }
        consume(samples)
    }
    deinit { stopInput(); stopOutput() }
}
