# WSJT-X-based JTTY client for the FT-710

Open `build/JTTY Workbench WSJTX.app`. This is the replacement Qt application, not the earlier Swift build. It uses an independent settings identity, so enter your station and radio settings once.

1. Open **File → Settings** (on macOS, the application preferences menu may also expose settings). Enter your callsign and grid under **General**.
2. Under **Radio**, select **Yaesu FT-710**, the radio's CAT serial port (previously `/dev/cu.usbserial-016B87F30`), and **115200** baud if that is still the radio's CAT rate. Use CAT for PTT, USB/Data mode as appropriate for the FT-710, and your known working WSJT-X serial parameters. Use **Test CAT** to verify control. Test PTT only when you intend to key the radio.
3. Under **Audio**, choose **USB Audio Device** for both input and output. This device reported one input channel and two output channels in the failed test. Use **Mono** input; choose your usual USB output channel. Save settings. macOS microphone permission covers USB input too; allow it.
4. JTTY is already selected. Choose the band/frequency from the upstream frequency selector (40 m defaults to 7.090 MHz). Confirm the displayed dial frequency before operation.
5. Use **Monitor** for reception. The waterfall and input level meter should show radio noise. Start with reception before sending a message. Upstream JTTY controls provide free text, queued messages and function-key macros; **Halt Tx** stops transmission.

Copy the actual audio and radio parameters from your working WSJT-X setup where needed. The app deliberately does not import or overwrite that application's settings.

Diagnostics:
`/Users/eduardo/Library/Application Support/JTTY Workbench/Logs/wsjtx_syslog.log`

Rotated logs are under the `logs/` subdirectory. The earlier Swift app's `diagnostics.log` remains available in the same parent directory. The Qt app records selected input, rate/channel, audio state/error, and upstream rig diagnostics. No successful physical receive/transmit test is claimed from the software loopback tests.

This workspace developer app uses external Homebrew libraries and is not a portable installer.
