# Version 0.2 verification — October 1, 2026

Built on this Apple Silicon Mac with GNU Fortran 16.2.0, FFTW 3.3.11, Hamlib 4.7.2, CMake 4.4.3 and installed Swift/Xcode tools.

- Four upstream core tests and 17 Python tests passed: WAV interoperability/validation, live message assembly, rolling receive buffer, Hamlib dummy CAT/PTT, tuning guards and independent timed release.
- Native audio harness passed a simulated 48 kHz stereo capture through channel selection, sample-rate conversion and JTTY decoding.
- Built-in MacBook speaker silent playback drained successfully. No radio output device was selected for this test.
- Native station harness passed queued transmissions, frequency/mode control, release, Tune cancellation, PTT test and shutdown against Hamlib's dummy radio. It used near-zero output on built-in speakers, no radio and no microphone.
- Prior WAV interface was visually verified. New Conversation controls were inspected through the native accessibility tree; subsequent setup-page UI inspection was blocked by the computer-use tool's native pipe failure.
- Upstream checkout remains unmodified.

No FT-710 serial or USB audio device was visible. Actual hardware CAT/PTT, microphone/USB capture permission flow, RF levels, weak-signal characterization and over-the-air interoperability remain unverified. A dummy backend is not evidence of actual radio behavior.

The app is a local developer bundle requiring its build-folder location and external Homebrew libraries. Packaging, notarization and installation on another Mac remain future work. A process-local PTT watchdog cannot cover forced termination or a disconnected cable.

The upstream protocol is unchanged. Complete marks receipt of an end marker, not a guarantee against false decodes. Future upstream protocol changes may require updating the pinned source.

Receive troubleshooting update: fixed an input callback race that attempted to enqueue during AudioQueueStop/Dispose (-66632). All 21 existing automated tests and native conversion passed afterward. A connected USB Audio Device (one input, two outputs, 48 kHz) was detected, but a receive-only start/stop hardware test failed at Start input (-66681); CoreAudio logged server start failures and a timeout. Real receive capture therefore remains unverified. No output or PTT was used in this test.
