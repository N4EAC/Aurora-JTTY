// JTTY Workbench station interface. GPL-3.0-or-later, 2026-10-01.
import SwiftUI
import AppKit

// Preliminary dial presets from DXZone, updated September 25, 2026.
struct JTTYDialPreset: Identifiable {
    let band: String
    let mhz: Double
    var id: String { band }
    static let all: [JTTYDialPreset] = [
        .init(band: "160 m", mhz: 1.838), .init(band: "80 m", mhz: 3.575),
        .init(band: "40 m", mhz: 7.090), .init(band: "30 m", mhz: 10.140),
        .init(band: "20 m", mhz: 14.090), .init(band: "17 m", mhz: 18.100),
        .init(band: "15 m", mhz: 21.090), .init(band: "12 m", mhz: 24.920),
        .init(band: "10 m", mhz: 28.090), .init(band: "6 m", mhz: 50.160),
        .init(band: "2 m", mhz: 144.160)
    ]
}

struct JTTYFrequencyControls: View {
    @ObservedObject var station: LiveStation
    private var presetLabel: String {
        if let preset = JTTYDialPreset.all.first(where: { abs($0.mhz - station.settings.dialMHz) < 0.0000005 }) {
            return String(format: "%@ · %.3f MHz", preset.band, preset.mhz)
        }
        return "Custom frequency"
    }
    var body: some View {
        HStack {
            Menu("JTTY: " + presetLabel) {
                ForEach(JTTYDialPreset.all) { preset in
                    Button(String(format: "%@ · %.3f MHz%@", preset.band, preset.mhz,
                                  preset.mhz > 60 ? " (outside FT-710 range)" : "")) {
                        station.settings.dialMHz = preset.mhz
                        station.settings.dataMode = true
                        station.save()
                    }.disabled(preset.mhz > 60)
                }
            }.frame(width: 235)
            Text("Dial MHz")
            TextField("14.090", value: $station.settings.dialMHz, format: .number.precision(.fractionLength(3...6))).frame(width: 110)
            Toggle("Data USB", isOn: $station.settings.dataMode)
            Button("Apply Frequency / Mode") { station.applyFrequency() }.disabled(!station.connected)
        }.disabled(station.transmitting || station.radioPTT)
        Text("Preliminary presets · Select, then Apply to tune the radio").font(.caption).foregroundStyle(.secondary)
    }
}

struct StationRootView: View {
    @ObservedObject var station: LiveStation
    var body: some View {
        TabView {
            ConversationView(station: station).tabItem { Text("Conversation") }
            StationSetupView(station: station).tabItem { Text("Station Setup") }
            WorkbenchView().tabItem { Text("WAV Files") }
        }.padding(10).frame(minWidth: 940, minHeight: 750)
    }
}

struct ConversationView: View {
    @ObservedObject var station: LiveStation
    var body: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Text("JTTY Workbench").font(.title.bold())
                Spacer()
                Button("Tune (3 s)") { station.tune() }.disabled(!station.connected || !station.transmitEnabled || station.transmitting)
                Text(station.dial).font(.title2.monospacedDigit())
                Text(station.radioMode).foregroundStyle(.secondary)
                Text(station.radioPTT ? "PTT ON" : "RX").foregroundStyle(station.radioPTT ? .red : .green)
            }
            JTTYFrequencyControls(station: station)
            Text(station.monitoring ? "Receiving: " + station.activeInput : "Selected input: " + (station.devices.first(where: { $0.uid == station.settings.inputUID })?.name ?? "Choose in Station Setup"))
                .font(.caption).foregroundStyle(.secondary)
            HStack {
                Button(station.monitoring ? "Stop Monitor" : "Monitor") { if station.monitoring { station.stopMonitor() } else { station.startMonitor() } }.disabled(station.transmitting)
                Text("Input").font(.caption)
                ProgressView(value: Double(station.peak), total: 1).frame(width: 100)
                Text(station.peak > 0.98 ? "CLIPPING" : String(format: "%.0f dBFS", 20 * log10(max(0.00001, station.peak)))).font(.caption.monospaced()).foregroundStyle(station.peak > 0.98 ? .red : .secondary)
                Spacer()
                Toggle("Enable transmit", isOn: $station.transmitEnabled).disabled(!station.connected || station.settings.pttMethod == 3)
                    .onChange(of: station.transmitEnabled) { enabled in if !enabled && station.transmitting { station.stop(reason: nil) } }
                Button("STOP / PTT OFF") { station.stop(reason: nil) }.tint(.red).buttonStyle(.borderedProminent)
            }
            WaterfallView(rows: station.waterfall, tone: $station.settings.tone).frame(height: 140)
                .disabled(station.transmitting)
            HStack {
                Text("Audio Hz")
                TextField("1500", value: $station.settings.tone, format: .number).frame(width: 75)
                Text("± Hz")
                TextField("100", value: $station.settings.tolerance, format: .number).frame(width: 65)
                Text("Click the waterfall to choose a signal").font(.caption).foregroundStyle(.secondary)
                Spacer()
                Picker("Exchange", selection: $station.settings.profile) {
                    Text("General text").tag("unknown")
                    Text("Field Day").tag("field-day")
                    Text("RTTY Roundup").tag("rtty-roundup")
                }.frame(width: 235)
            }.disabled(station.transmitting)
            ScrollViewReader { scroll in
                ScrollView {
                    LazyVStack(alignment: .leading, spacing: 10) {
                        if station.conversation.isEmpty {
                            Text("Incoming messages appear here as they arrive. Select USB audio in Station Setup, then press Monitor.")
                                .foregroundStyle(.secondary).padding(14)
                        }
                        ForEach(station.conversation) { item in
                            HStack(alignment: .top) {
                                Text(item.direction).font(.caption.bold()).foregroundStyle(item.direction == "TX" ? .orange : .green).frame(width: 30)
                                VStack(alignment: .leading, spacing: 3) {
                                    Text(item.text).font(.system(.body, design: .monospaced)).textSelection(.enabled)
                                    Text("\(item.date.formatted(date: .omitted, time: .standard)) · \(Int(item.frequency)) Hz\(item.complete ? "" : " · receiving…")")
                                        .font(.caption).foregroundStyle(.secondary)
                                }
                                Spacer()
                            }.padding(.horizontal, 12).id(item.id)
                        }
                    }.padding(.vertical, 12)
                }.background(Color(nsColor: .textBackgroundColor)).clipShape(RoundedRectangle(cornerRadius: 8))
                    .onChange(of: station.conversation.count) { _ in if let last = station.conversation.last { scroll.scrollTo(last.id, anchor: .bottom) } }
            }
            HStack {
                TextField("Their callsign", text: $station.settings.theirCall).frame(width: 140)
                TextField("Queued callsign", text: $station.settings.queuedCall).frame(width: 140)
                Stepper("Serial \(station.settings.serialNumber)", value: $station.settings.serialNumber, in: 1...131071)
                Spacer()
                Button("Log Contact") { station.logContact() }.disabled(!station.connected || station.settings.theirCall.isEmpty || station.transmitting)
            }
            HStack {
                ForEach(0..<8, id: \.self) { index in
                    Button("F\(index + 1)") { station.macro(index) }
                        .help(station.settings.macros[index])
                        .keyboardShortcut(KeyEquivalent(Character(UnicodeScalar(0xF704 + index)!)), modifiers: [])
                }
                Spacer()
                Text("Queued: \(station.queue.count)").font(.caption)
            }.disabled(!station.transmitEnabled || !station.connected)
            HStack {
                TextField("Type a message (80 characters max)", text: $station.draft)
                    .onSubmit { station.enqueue() }
                Text("\(station.draft.count)/80").font(.caption.monospaced())
                Button(station.transmitting ? "Queue" : "Send") { station.enqueue() }.buttonStyle(.borderedProminent)
                    .disabled(!station.connected || !station.transmitEnabled || station.draft.isEmpty)
            }
            if !station.queue.isEmpty {
                Text(station.queue.joined(separator: " → ")).lineLimit(1).font(.caption).foregroundStyle(.secondary)
            }
            Text(station.status).font(.callout)
            if !station.failure.isEmpty { Text(station.failure).foregroundStyle(.red).font(.callout).textSelection(.enabled) }
        }.padding(18)
    }
}

struct WaterfallView: View {
    let rows: [[Float]]
    @Binding var tone: Double
    var body: some View {
        GeometryReader { geometry in
            ZStack(alignment: .topLeading) {
                Canvas { context, size in
                    context.fill(Path(CGRect(origin: .zero, size: size)), with: .color(Color(red: 0.02, green: 0.04, blue: 0.12)))
                    let height = max(1, size.height / 90)
                    for (row, values) in rows.reversed().enumerated() {
                        let width = size.width / Double(max(1, values.count))
                        for (column, value) in values.enumerated() {
                            let color = Color(hue: 0.68 - Double(value) * 0.55, saturation: 0.9, brightness: 0.08 + Double(value) * 0.92)
                            context.fill(Path(CGRect(x: Double(column) * width, y: Double(row) * height, width: width + 0.2, height: height + 0.2)), with: .color(color))
                        }
                    }
                    let x = (tone - 200) / 2600 * size.width
                    var line = Path(); line.move(to: CGPoint(x: x, y: 0)); line.addLine(to: CGPoint(x: x, y: size.height))
                    context.stroke(line, with: .color(.yellow), lineWidth: 1)
                }
                HStack { Text("200 Hz"); Spacer(); Text("1500 Hz"); Spacer(); Text("2800 Hz") }.font(.caption.monospaced()).foregroundStyle(.white).padding(7)
                if rows.isEmpty { Text("Waterfall · start Monitor to receive audio").foregroundStyle(.gray).frame(maxWidth: .infinity, maxHeight: .infinity) }
            }.clipShape(RoundedRectangle(cornerRadius: 8)).contentShape(Rectangle())
                .gesture(DragGesture(minimumDistance: 0).onChanged { gesture in tone = min(2600, max(200, 200 + gesture.location.x / geometry.size.width * 2600)) })
        }
    }
}

struct StationSetupView: View {
    @ObservedObject var station: LiveStation
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                Text("Station Setup").font(.title.bold())
                Text("FT-710 USB: use the Enhanced serial port for CAT and the USB audio device for input and output. Close WSJT-X/fldigi's radio connection while this app owns the port.")
                    .foregroundStyle(.secondary)
                GroupBox("Radio · Hamlib") {
                    VStack(alignment: .leading, spacing: 10) {
                        HStack {
                            Picker("Radio", selection: $station.settings.model) {
                                Text("Yaesu FT-710").tag(1049)
                                Text("Network rigctld").tag(2)
                                Text("Dummy radio (testing)").tag(1)
                            }.frame(width: 290)
                            Picker("PTT", selection: $station.settings.pttMethod) {
                                Text("CAT (USB data audio)").tag(0)
                                Text("RTS").tag(1)
                                Text("DTR").tag(2)
                                Text("None · receive only").tag(3)
                            }.frame(width: 280)
                        }
                        HStack {
                            TextField(station.settings.model == 2 ? "localhost:4532" : "/dev/cu.usbserial…", text: $station.settings.serialPath)
                            if !station.ports.isEmpty {
                                Menu("Detected ports") { ForEach(station.ports, id: \.self) { port in Button(port) { station.settings.serialPath = port } } }
                            }
                            Picker("Baud", selection: $station.settings.baud) {
                                ForEach([4800,9600,19200,38400,115200], id: \.self) { value in Text("\(value)").tag(value) }
                            }.frame(width: 160)
                            Picker("Stop bits", selection: $station.settings.stopBits) { Text("1").tag(1); Text("2").tag(2) }.frame(width: 120)
                        }
                        if station.settings.pttMethod == 1 || station.settings.pttMethod == 2 {
                            HStack {
                                Text("PTT port (Standard USB)")
                                TextField("/dev/cu.usbserial…", text: $station.settings.pttPath)
                                Menu("Detected ports") { ForEach(station.ports, id: \.self) { port in Button(port) { station.settings.pttPath = port } } }
                            }
                        }
                    }.disabled(station.connected || station.connecting).padding(8)
                    HStack {
                        Button(station.connected ? "Disconnect" : station.connecting ? "Connecting…" : "Connect / Test CAT") { if station.connected { station.disconnect() } else { station.connect() } }.disabled(station.connecting || station.transmitting)
                        Text(station.dial + " " + station.radioMode).foregroundStyle(station.connected ? .green : .secondary)
                        Button("Test PTT (1 s)") { station.testPTT() }.disabled(!station.connected || !station.transmitEnabled || station.transmitting)
                        Spacer()
                        Button("Refresh Devices") { station.refresh() }.disabled(station.monitoring || station.transmitting)
                    }.padding(8)
                }
                GroupBox("USB audio") {
                    VStack(alignment: .leading, spacing: 12) {
                        HStack {
                            Picker("Input", selection: $station.settings.inputUID) {
                                Text("Choose input…").tag("")
                                ForEach(station.devices.filter { $0.inputs > 0 }) { Text($0.name).tag($0.uid) }
                            }
                            Picker("Channel", selection: $station.settings.inputChannel) {
                                ForEach(0..<max(1, station.devices.first(where: { $0.uid == station.settings.inputUID })?.inputs ?? 1), id: \.self) { index in Text("\(index + 1)").tag(index) }
                            }.frame(width: 120)
                        }
                        HStack {
                            Picker("Output", selection: $station.settings.outputUID) {
                                Text("Choose output…").tag("")
                                ForEach(station.devices.filter { $0.outputs > 0 }) { Text($0.name).tag($0.uid) }
                            }
                            Picker("Channel", selection: $station.settings.outputChannel) {
                                ForEach(0..<max(1, station.devices.first(where: { $0.uid == station.settings.outputUID })?.outputs ?? 1), id: \.self) { index in Text("\(index + 1)").tag(index) }
                            }.frame(width: 120)
                        }
                        HStack { Text("Transmit level \(Int(station.settings.txGain * 100))%"); Slider(value: $station.settings.txGain, in: 0...1).frame(width: 250); Text("Start low; adjust using the radio's ALC meter.").font(.caption).foregroundStyle(.secondary) }
                        HStack {
                            Text("PTT lead ms"); TextField("200", value: $station.settings.leadMS, format: .number).frame(width: 90)
                            Text("PTT tail ms"); TextField("150", value: $station.settings.tailMS, format: .number).frame(width: 90)
                        }
                    }.padding(8).disabled(station.monitoring || station.transmitting)
                }
                GroupBox("Station and dial frequency") {
                    HStack {
                        TextField("My callsign", text: $station.settings.myCall).frame(width: 150)
                        TextField("My grid", text: $station.settings.myGrid).frame(width: 100)
                        Spacer()
                    }.padding(8).disabled(station.transmitting)
                    VStack(alignment: .leading) {
                        JTTYFrequencyControls(station: station)
                        Link("JTTY frequency reference (DXZone)", destination: URL(string: "https://www.dxzone.com/jtty-frequencies/")!).font(.caption)
                    }.padding(8)
                }
                DisclosureGroup("Edit F1–F8 messages") {
                    ForEach(0..<8, id: \.self) { index in
                        HStack { Text("F\(index + 1)").frame(width: 30); TextField("Message", text: $station.settings.macros[index]) }
                    }
                    Text("%M = MyCall · %H = TheirCall · %Q = queued callsign · %E = 599 plus serial number").font(.caption).foregroundStyle(.secondary)
                }
                HStack {
                    Button("Save Settings") { station.save(); station.status = "Settings saved" }
                    Button("Open Logs") {
                        try? FileManager.default.createDirectory(at: station.logFolder, withIntermediateDirectories: true)
                        NSWorkspace.shared.open(station.logFolder)
                    }
                    Spacer()
                    Text("PTT timeout: 45 seconds maximum").font(.caption).foregroundStyle(.secondary)
                }
                if !station.failure.isEmpty { Text(station.failure).foregroundStyle(.red).textSelection(.enabled) }
                Text("Independent client based on WSJT-X · GPLv3 or later · Hardware validation required on your FT-710.").font(.caption).foregroundStyle(.secondary)
            }.padding(20)
        }
    }
}
