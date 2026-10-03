# Developing Aurora JTTY

## Source organization

- `upstream/wsjtx`: unchanged pinned source submodule, including the JTTY codec, Qt audio, transceiver support and upstream license notices.
- `scripts/prepare-wsjtx-jtty.py`: creates the generated source tree and applies Aurora overlays.
- `scripts/customize-jtty-ui.py`, `scripts/brand-aurora-jtty.py`, `native/qt-ui/`: application identity, layout, themes and message controls.
- `scripts/add-jtty-snr.py`: receive SNR reporting.
- `scripts/limit-jtty-fallback.py`: restricts the final half-symbol fallback to its highest-ranked hypothesis while retaining the primary candidate search, CRC and source validation. This trades some fallback sensitivity for fewer false acceptances.
- `scripts/deepen-jtty-debug.py`, `native/qt-audio/`: audio diagnostics and macOS device-route monitoring.
- `client/jtty.py`, `native/engine.f90`, `native/live_bridge.f90`: recorded-audio and codec interfaces.
- `native/Assets`: app icon assets. The Swift files and `scripts/build.sh` retain the earlier client implementation for developers.

Do not edit generated files under `build/` or modify the pinned upstream checkout directly. Make changes in the maintained overlays, then rebuild to verify reproducibility.

## Build and test

Use the dependency installation and build commands in README.md. `scripts/test-wsjtx-jtty.sh` runs GUI startup, live-audio fixtures, transmit loopback, codec, receive-state and message-matching tests. These use software fixtures; they do not establish physical USB or over-the-air operation.

Verify layout at different window sizes and themes when changing the interface. Retain existing widget signals, frequency/PTT controls, decoder behavior and settings compatibility. For radio changes, separately test CAT, selected USB input/output and PTT release with suitable station equipment.

See [audio diagnostics](../native/qt-audio/DEBUGGING.md) for log locations and events. Avoid committing station logs, personal recordings, device identifiers or credentials.

## Release packaging

Build the application, then run `sh scripts/package-macos-dmg.sh`. The packaging script includes corresponding source, legal notices and a SHA-256 checksum. Verify the DMG and its source archive before uploading assets. GitHub's automatic source downloads exclude submodule contents and do not replace the complete release source archive.

Preserve the requirements in COPYING, THIRD_PARTY_NOTICES.md and [distribution requirements](PUBLICATION-REQUIREMENTS.md). Bundled third-party binaries need their applicable notices and source obligations. Signing, notarization and self-contained dependency deployment are separate packaging steps.

## Windows build

Run the **Build Aurora JTTY Windows** workflow from GitHub Actions. It uses a Windows runner with MSYS2 MINGW64, builds Hamlib 4.7.2, applies the maintained overlays, compiles the client and creates an NSIS installer plus a portable ZIP. The workflow lists the exact build dependencies.

For a local MSYS2 MINGW64 environment with those dependencies installed, build Hamlib into `build/hamlib-prefix`, retain its source in `build/hamlib-src`, then run `bash scripts/build-windows-jtty.sh` and `python scripts/package-windows-jtty.py`. Run `scripts/test-windows-startup.ps1` from PowerShell and `makensis scripts/windows-installer.nsi` from MINGW64. Packaging collects runtime DLLs, notices and matching dependency source packages; keep these source packages available when distributing binaries.

The workflow verifies both the portable executable and the installed executable without the development DLL search paths. Test physical CAT/PTT and audio separately on Windows 10/11 x64 before treating this preview as a validated station release.
