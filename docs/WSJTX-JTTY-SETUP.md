# Aurora JTTY station setup

1. Open Aurora JTTY and **Radio / Audio**. Enter your callsign and grid in the General settings.
2. In Radio settings, select your Hamlib radio model and CAT serial port. Match the baud rate, stop bits and handshake to the radio. Select CAT PTT when supported. **Test CAT** checks control; use PTT/Tune only when you intend to transmit.
3. In Audio settings, select the radio's USB input and output separately. Use Mono input for a one-channel USB interface. Save settings. Allow macOS microphone permission, which also covers USB audio inputs.
4. Select the band/frequency and confirm the displayed dial frequency and CAT mode. Press **Monitor**. The input meter and waterfall should respond to radio audio.
5. Click a signal in the waterfall or select a received message to set the receive offset. Conversation follows Rx ± tolerance. Use the copy buttons to match receive and transmit offsets when needed.
6. Type in Message and press Enter or Send message. Configure macro templates and use F1–F8 for repeated messages. Halt Tx stops transmission. Log QSO opens the contact logging workflow.

For the FT-710, choose **Yaesu FT-710**, its CAT USB serial port, and serial parameters matching the radio. Select its **USB Audio Device** for input/output. USB serial port identifiers differ between computers. Use your radio's USB modulation and output-level controls to set audio levels; avoid clipping and excessive ALC.

**Yaesu FT-710 on macOS:** In **Audio MIDI Setup**, select the radio’s **USB Audio Device** and set **Format to 44,100 Hz for both input and output**, instead of 48,000 Hz. This setting is required for the FT-710 to work with Aurora JTTY on macOS. Stop monitoring before changing the format, then reselect the USB input/output in **Radio / Audio** and restart Monitor.

If USB input is unavailable, stop monitoring, reconnect the device, reopen Audio settings and select it again. Check macOS microphone permission and the device format in Audio MIDI Setup. CAT and audio are separate USB functions. Close other applications using the CAT port while configuring this app.

## Files and diagnostics

Settings retain the JTTY Workbench identity. Recordings are under `~/Library/Application Support/JTTY Workbench/save`; app logs are under `~/Library/Application Support/JTTY Workbench/Logs`. See [audio diagnostics](../native/qt-audio/DEBUGGING.md) for developer logging details.

The app uses its own settings and does not import or overwrite an installed WSJT-X application's configuration.
