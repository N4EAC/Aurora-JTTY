# FT-710 USB setup and first JTTY conversation

The app is built for this Mac. Open `build/JTTY Workbench.app`; quit and reopen any older running copy after rebuilding.

## Connect the station

1. Connect and power on the FT-710 via USB. Close WSJT-X and fldigi while using the same serial port here. Their working settings are useful references.
2. In **Station Setup**, refresh devices. Select **Yaesu FT-710**, its Enhanced USB serial port, and **CAT** PTT. Use the baud rate and stop bits already configured on the radio. The FT-710 CAT1 defaults are 38400 baud and one stop bit; do not change a working setup just to match the app's defaults. The app uses no serial hardware handshake.
3. Press **Connect/Test CAT**. A successful connection displays frequency and mode. Connecting does not change the radio's frequency or key it. If serial ports are missing, check the USB cable and the same USB serial driver used by your existing applications.
4. Select the radio's USB audio input and USB audio output separately. Choose the appropriate channel, usually channel 1. Keep the Mac's speakers/microphone out of the radio audio path. Set your callsign and grid, then save settings.
5. On the radio, use DATA USB and set **RADIO SETTING → MODE PSK/DATA → MOD SOURCE** to USB. Use the radio's **USB MOD GAIN** and **USB OUT LEVEL** controls together with the app's transmit level and receive meter. When using CAT PTT, follow Yaesu's CAT manual guidance for **RPTT SELECT = OFF**. RTS/DTR PTT is an alternative requiring the correct Standard USB PTT port and matching radio menu configuration.
6. Use the **JTTY** frequency menu in Conversation or Station Setup, or enter a custom dial frequency in MHz. Press **Apply Frequency / Mode** to tune the connected radio. Selecting a preset alone saves the choice without tuning. The preliminary [DXZone list](https://www.dxzone.com/jtty-frequencies/) supplies presets from 160 m through 6 m; its 2 m entry is disabled because it is outside the FT-710 range. The initial 20 m selection is 14.090 MHz. Coordinate a suitable frequency with the other station and follow your local band plan.

Reference: [Yaesu FT-710 CAT manual](https://www.yaesu.com/Files/4CB893D7-1018-01AF-FA97E9E9AD48B50C/FT-710_CAT_OM_ENG_2306-C.pdf), [Hamlib FT-710 backend](https://github.com/Hamlib/Hamlib/blob/master/rigs/yaesu/ft710.c).

## Receive first

In **Conversation**, press **Monitor** and allow macOS audio input access. The receive meter and waterfall should respond to radio audio. Avoid clipping. Set the lowest audio tone near 1500 Hz initially; clicking the waterfall selects a tone. Start with a 100 Hz receive tolerance and match the other station's signal. A narrower tolerance reduces the search range.

Messages appear progressively. A partial message has not yet received its end marker; even a complete decode is not an assurance against false decoding. Reception is paused during your own transmission and resumes afterward if it was running.

## Send a conversation

JTTY is keyboard-to-keyboard: type a message, then queue it. It does not use FT8's alternating clock slots. Messages are uppercased and limited to 80 supported characters. The app sends queued messages in order and returns to receive after each transmission.

Before the first transmission, select **Enable Transmit**. **Test PTT** keys the actual radio for one second without modulation. **Tune** sends a three-second tone. Use low radio power for initial level adjustment and watch its ALC meter; adjust the app output level and radio USB modulation gain for a clean signal. **Stop/PTT OFF** cancels the queue and attempts immediate release. The app also has a 45-second maximum PTT deadline.

A simple exchange is: `CQ YOURCALL CQ`, then `OTHERCALL YOURCALL`, then a short signal report and conversation, followed by `73 YOURCALL`. Replace the placeholder calls with actual callsigns. F1–F8 invoke editable macros; set your call and the other station's call first. The queued-call and serial fields support contest-style exchanges. Both stations should use compatible JTTY code, such as the pinned WSJT-X release candidate.

**Log Contact** explicitly records the entered other callsign, current CAT frequency and UTC time; it does not infer a completed QSO from received text. It also advances the serial number. **Open Logs** reveals JSONL conversation records and `contacts.adi` in `~/Library/Application Support/JTTY Workbench/Logs`.

JTTY is absent from the [ADIF 3.1.7 mode list](https://adif.org.uk/317/ADIF_317.htm). Contact exports use the broad MFSK mode and preserve JTTY in an application field and comment; verify how your logging software imports these records.

## Current validation limits

The encoder/decoder, native sample-rate conversion, transmit queue, Hamlib dummy CAT/PTT, cancellation and timed release have been tested. Live receive-only capture and repeated restart now pass on the connected USB audio device using PortAudio. The user has reported working CAT. Modulation levels, actual PTT and a two-station contact remain to be checked. The watchdog cannot release PTT if the process is forcibly killed or the control cable fails; use the radio's own controls in that situation.

## USB receive troubleshooting

CAT and USB audio are separate devices. Select **USB Audio Device** as input and channel 1, then press Monitor. If CoreAudio cannot start input, stop audio monitoring in WSJT-X/fldigi, reconnect the radio's USB cable, refresh devices and retry. Check JTTY Workbench under macOS Privacy & Security → Microphone. The AudioQueue input path produced -66632/-66681 on this device. The updated build uses PortAudio for capture, matching the backend selected in fldigi, and has passed live USB receive and restart tests. Quit and reopen the app to load the update. If monitoring runs but the meter remains silent, check the radio's USB OUT LEVEL and selected audio source.
