# Failed radio test and architecture replacement

Production diagnostics inspected on October 2, 2026. Times below are UTC (local New York time is four hours earlier).

- 09:58:47: FT-710 Hamlib model 1049 connected. Serial DTR/RTS OFF. Repeated polls succeeded, frequency 7.090 MHz, PKTUSB, PTT off.
- 09:59:05: capture requested selected USB UID `AppleUSBAudioEngine:C-Media Electronics Inc.:USB Audio Device:2142000:2,1`, CoreAudio ID 123, mono, 48 kHz. Qt helper selected USB, reported Idle and 48000/16/1 format.
- 09:59:09: selected USB ID 123 reported alive=0, status=0. Capture guard stopped it. CAT polls continued successfully.
- 09:59:17: parent enumerated same USB UID at a NEW ID 162; new helper still resolved ID 123 and stopped again. This is evidence of device disappearance/reappearance and inconsistent process device enumeration, not proof of its cause.

User explicitly requested the fallback: full WSJT-X-based JTTY-only app. The new build uses the existing pinned GPL upstream submodule through a reproducible source overlay (`scripts/prepare-wsjtx-jtty.py`), preserving the original submodule. It uses WSJT-X MainWindow, Configuration, audio input/output, Detector/Modulator, JTTY messaging, and Hamlib transceiver implementation in one Qt application. No Swift frontend, subprocess audio pipe, custom output queue, or custom CAT C wrapper participates.

Other mode menu actions and quick buttons are hidden/disabled; their mode selection slots are guarded. Restored profiles, central mode selection, and command-line mode selection force JTTY. The common upstream DSP library remains intact because removing shared symbols would require a separate larger dependency refactor.

Application identity is `JTTY Workbench`, bundle identifier `com.n4eac.jtty.wsjtx`, isolated from WSJT-X and the old Swift bundle. Original WSJT-X attribution is retained. USB input opening/state and CAT diagnostics are logged through upstream Boost logging. Hardware success remains unverified until a user radio test.

## Verification

The replacement compiled successfully against Qt 5.15.19, Boost 1.92, Hamlib 4.7.2 and GCC 16.2. Initial startup testing caught the unused multimode decoder shared-memory allocation exceeding macOS limits. The fork removes that allocation and subprocess launch because upstream JTTY uses in-process fast decoding; no system shared-memory limits were changed.

Six selected upstream tests passed (60.64 seconds total): application startup including forced JTTY after an FT8 command-line request, JTTY live audio decoding, queued JTTY transmit WAV capture and replay decoding, source codec, message matching, and receive state. They ran with offscreen Qt and isolated test settings, without RF transmission. The developer bundle identity and original icon were verified.

Replacement app: `build/JTTY Workbench WSJTX.app`.
New production log: `/Users/eduardo/Library/Application Support/JTTY Workbench/Logs/wsjtx_syslog.log`; rotations are under the `logs/` subdirectory. Old Swift diagnostics remain in `diagnostics.log`.

Hardware audio continuity, selected physical channel, CAT/PTT and RF operation remain to be validated by the user. The common DSP library still contains other mode routines, but this application's operating mode is restricted to JTTY.
