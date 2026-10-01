# JTTY Workbench

An independent macOS JTTY client using the unmodified WSJT-X JTTY core, SwiftUI, CoreAudio and Hamlib. GPLv3 or later. Version 0.2 provides live receive, waterfall, audio device/channel selectors, a transmit queue, eight editable macros, CAT frequency/mode control, PTT, tune, contact logging and WAV tools.

Open `build/JTTY Workbench.app`. Follow [the FT-710 setup guide](docs/FT-710-SETUP.md) before transmitting. Start with receive monitoring. Transmit is disabled at launch until explicitly enabled.

Software and simulated-radio tests pass. Actual FT-710 USB operation and over-the-air interoperability still require hardware validation; the radio is not currently visible on this Mac. This is a development client, not a released installer.

## Build

With Xcode command-line tools and Homebrew:

```sh
brew install gcc fftw cmake hamlib
sh scripts/build.sh
sh scripts/test.sh
open "build/JTTY Workbench.app"
```

Keep the developer bundle in this workspace's `build/` directory. It depends on Homebrew libraries and the local Python client. It is not packaged or notarized for distribution.

Clone this repository with `git clone --recurse-submodules` or run `git submodule update --init --recursive` after cloning.

The required upstream checkout is pinned to `v3.2.0-rc1`, commit `567ad29ce6abf3d4a44f181cdbc7ceba0d73e5f4`. To restore it:

```sh
git clone --depth 1 --branch v3.2.0-rc1 https://github.com/WSJTX/wsjtx.git upstream/wsjtx
```

## Recorded-audio CLI

```sh
python3 client/jtty.py encode "CQ K1ABC CQ" build/cq.wav
python3 client/jtty.py decode build/cq.wav
python3 client/jtty.py decode upstream/wsjtx/samples/JTTY/260807_134110.wav
```

Input WAV files must be 12 kHz mono PCM16, at most 180 seconds. Messages support up to 80 characters and use upstream normalization. Output is JSON. Live audio converts the selected device's sample rate to the same decoder format.

## Source and verification

- `native/Audio.swift`: device selection, capture, playback and conversion.
- `native/Station.swift` and `LiveView.swift`: station workflow and interface.
- `native/radio.c`: Hamlib commands, serialization and timed PTT release.
- `native/live_bridge.f90`: streaming upstream decoder and encoder bindings.
- `native/engine.f90` and `client/jtty.py`: recorded-audio tools.
- `research/`: licensing, pinned provenance and verification results.

The PTT watchdog releases a timed transmission while this process and the control connection remain functional. It cannot guarantee release after forcible process termination or a disconnected USB cable. Stop/PTT OFF and graceful shutdown attempt immediate release and report failures.

See [verification details](research/PROTOTYPE-STATUS.md), `COPYING`, `THIRD_PARTY_NOTICES.md`, and [licensing research](research/JTTY-LICENSING.md). Preserve notices and provide corresponding source when distributing covered binaries. No upstream endorsement is claimed.

The original app icon is in `native/Assets`; regenerate it with `swift scripts/make-icon.swift native/Assets` followed by `iconutil -c icns native/Assets/JTTY.iconset -o native/Assets/JTTY.icns`.
