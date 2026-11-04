# Aurora JTTY 1.3 — macOS Apple Silicon developer build

This DMG contains the current arm64 application, source snapshot and legal notices. It is not a standalone or notarized public installer and does not support Intel Macs. Keep the included source archive with any redistribution.

Install the required Homebrew dependencies before opening the app:

```sh
brew install gcc fftw hamlib qt@5 boost libusb
```

The binaries currently reference Homebrew libraries under `/opt/homebrew`; compatible installed library versions are required. Drag Aurora JTTY.app to Applications, or run it from a writable local folder. macOS may require approval to open this unsigned developer application. Microphone permission is required for USB radio audio input as well as microphones.

**Yaesu FT-710 on macOS:** In **Audio MIDI Setup**, select the radio’s **USB Audio Device** and set **Format to 44,100 Hz for both input and output**, instead of 48,000 Hz. This setting is required for the FT-710 to work with Aurora JTTY on macOS. Stop monitoring before changing the format, then reselect the USB input/output in **Radio / Audio** and restart Monitor.

Select your radio, CAT port, USB input and USB output in Radio / Audio. The existing JTTY Workbench settings and save/log folders are retained. The DMG does not change radio settings or transmit automatically.

See THIRD_PARTY_NOTICES.md, COPYING and PUBLICATION-REQUIREMENTS.md for software copyright, dependency obligations and release requirements. The corresponding source archive contains the pinned upstream source, Aurora modifications and build scripts; build instructions are in its README.md. External dependencies remain separately installed and their binaries are not bundled.

The source snapshot omits unrelated mode sample recordings, QDarkStyle demonstration screenshots and unused upstream Windows binaries; build source and JTTY samples are retained. The full unchanged upstream checkout remains available at the pinned commit identified in README.md.

Version 1.3 uses monospace fonts and offers Default and Greenscreen in View → Themes. The optional beacon transmits CALL BEACON GRID at 5–60 minute intervals (5-minute steps). It starts disabled each launch; see the README for setup and operating behavior.

Development builds are staged in separate directories rather than replacing a running app bundle. The whole bundle is ad hoc signed and verified, but is not Developer ID signed or notarized. macOS may ask for microphone permission once again after a changed development build; repeated requests during one unchanged running build are not expected. Quit the old app before opening a new build or replacing the installed copy.
