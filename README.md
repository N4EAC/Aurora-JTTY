
<img width="1609" height="977" alt="Bunker Ham Shack Digital Signals" src="https://github.com/user-attachments/assets/b6552f1a-39cc-4efb-bdbc-f7f3177ce1ff" />

# Aurora JTTY 1.0

![Platform: macOS Apple Silicon](https://img.shields.io/badge/platform-macOS%20Apple%20Silicon-blue)
![Platform: Windows x64](https://img.shields.io/badge/platform-Windows%20x64-blue)
<img width="1728" height="1049" alt="Screenshot 2026-10-02 at 21 12 44" src="https://github.com/user-attachments/assets/c8dae4f8-3ae6-4107-8dde-a009879cd19e" />
Aurora JTTY is an independent JTTY-only amateur-radio text client based on WSJT-X, with Hamlib CAT/PTT control, USB audio selection, a waterfall and editable message macros.

## Download and install

[macOS release](https://github.com/N4EAC/Aurora-JTTY/releases/tag/v1.0) · [Apple Silicon DMG](https://github.com/N4EAC/Aurora-JTTY/releases/download/v1.0/Aurora-JTTY-1.0-macOS-arm64.dmg) · [Installation instructions](docs/MACOS-INSTALL.md)

The current release is an unsigned Apple Silicon developer build requiring compatible Homebrew libraries. It is not a standalone or notarized installer. The release includes a checksum, corresponding source archive and legal notices.

[Windows x64 preview release](https://github.com/N4EAC/Aurora-JTTY/releases/tag/v1.0-windows-preview) · [Windows installer](https://github.com/N4EAC/Aurora-JTTY/releases/download/v1.0-windows-preview/Aurora-JTTY-1.0-windows-x64-setup.exe) · [Windows installation instructions](docs/WINDOWS-INSTALL.md)

The Windows preview includes its runtime dependencies and is unsigned. Native Windows startup and installation are checked automatically; radio USB audio and CAT/PTT require testing with your station. Complete application and dependency source packages accompany the release.

## Using Aurora JTTY

Configure your callsign, grid, radio, CAT serial port and USB audio input/output in **Radio / Audio**. See [station setup](docs/WSJTX-JTTY-SETUP.md). Select a band and confirm the dial frequency before pressing **Monitor**.

- **All Messages** shows decoded signals across the waterfall range, with frequency and estimated SNR.
- **Conversation** follows the selected receive offset and tolerance. It can open in a dedicated window.
- Type in **Message** and use Enter or **Send message** to transmit. F1–F8 invoke editable macros. **Halt Tx** stops transmission.
- The waterfall selects the receive offset. Use **Copy Rx → Tx** or **Copy Tx → Rx** to match offsets.
- **View → Themes** selects Light gray, Dark or Aurora. **Message font size** sets message text to 8–17 pt.

## Build from source

Install Xcode command-line tools and Homebrew, then:

```sh
git clone --recurse-submodules https://github.com/N4EAC/Aurora-JTTY.git
cd Aurora-JTTY
brew install gcc fftw cmake hamlib qt@5 boost libusb portaudio
sh scripts/build-wsjtx-jtty.sh
sh scripts/test-wsjtx-jtty.sh
open "build/Aurora JTTY.app"
```

For an existing clone, run `git submodule update --init --recursive`. The upstream checkout is pinned at `567ad29ce6abf3d4a44f181cdbc7ceba0d73e5f4` (`v3.2.0-rc1`). GitHub's automatically generated source archives omit submodule contents; use the complete source archive attached to the release or clone with submodules.

## Development

See [the development guide](docs/DEVELOPMENT.md) for source organization, tests and packaging. The reproducible overlay in `scripts/prepare-wsjtx-jtty.py` generates the Qt client from the unchanged pinned upstream source. Change maintained overlay scripts and assets instead of editing the generated `build/wsjtx-jtty-source` directory.

```sh
sh scripts/package-macos-dmg.sh
```

This creates the DMG and checksum under `installers/`, with the application, corresponding source and legal notices. The current package uses external Homebrew libraries; bundling dependencies, signing and notarization require additional release work.

## Recorded-audio tools

```sh
python3 client/jtty.py encode "CQ K1ABC CQ" build/cq.wav
python3 client/jtty.py decode build/cq.wav
```

Decoder input is 12 kHz mono PCM16 WAV, up to 180 seconds. Messages support up to 80 characters; output is JSON. Live audio is converted to the decoder format.

## License and attribution

Aurora JTTY is licensed under **GNU GPL version 3 or later**. It is based on WSJT-X and is not endorsed by the WSJT Development Group. Original copyright notices are preserved.

- [Full license](COPYING)
- [Third-party licenses and attribution](THIRD_PARTY_NOTICES.md)
- [Distribution requirements](docs/PUBLICATION-REQUIREMENTS.md)

Binary distributions must provide complete corresponding source, required notices and applicable dependency compliance. Upstream end-user guides are separately licensed CC BY-ND 4.0; redistribute them unmodified with attribution or write independent documentation.
