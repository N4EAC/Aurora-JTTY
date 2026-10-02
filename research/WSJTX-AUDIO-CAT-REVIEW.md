# WSJT-X audio/CAT comparison — October 1, 2026

Reviewed the pinned, local WSJT-X v3.2.0-rc1 source rather than assuming its architecture.

## Audio input

`upstream/wsjtx/Audio/soundin.cpp`, SoundInput::start, opens QAudioInput with the explicit configured QAudioDeviceInfo and signed PCM16 at 12000 × downsampleFactor. It requests mono or stereo based on channel selection and checks format support and stream errors. It reports input stream state and format. Suspend/resume uses stop/restart, with a reset of the receive sink.

`upstream/wsjtx/widgets/mainwindow.cpp` moves SoundInput to a dedicated audio thread and wires start, suspend, resume, stop, error and stream-descriptor signals. The settings handler selectively reopens audio when the input/source configuration changes and restores monitoring state. CAT configuration is a separate subsystem.

Our application uses a direct CoreAudio input IOProc and native Float32 samples converted to mono 12 kHz. That is a separate implementation; it does not inherit WSJT-X's proven device backend. Its callback and serial processing queue separate capture/conversion from the GUI, but input setup/teardown is currently initiated on the main thread. The backend and hardware behavior remain experimental.

## Serial CAT and PTT

`upstream/wsjtx/Transceiver/HamlibTransceiver.cpp` separately configures serial speed, handshake, optional forced DTR/RTS states, PTT type/port and shared-port policy before rig_open. The previous JTTY wrapper set no handshake but did not explicitly force the CAT port's DTR/RTS lines. Those defaults can matter when a radio treats a serial line as PTT.

The wrapper now forces DTR and RTS OFF on the CAT serial port. RTS/DTR PTT continues to use the separately specified PTT port. Both configuration options were accepted by Hamlib's FT-710 backend without opening a physical port. This is a plausible correction for CAT-open receive muting, not a confirmed diagnosis of the user's hardware symptom. It does not explain all earlier microphone-routing failures.

## Regression verification

`tests/ReceiveCATHarness.swift` injects a continuous synthetic receive source into the real LiveStation, while using real Hamlib model 1. It verifies that CAT connect, several polls, and frequency/mode changes do not stop or recreate monitoring, that samples continue reaching the receive pipeline, and that shutdown stops it. It issues no PTT-on command and accesses no hardware, microphone or speakers.

Four core tests, 17 Python tests, native conversion and the new receive/CAT lifecycle harness passed. Passing simulated tests does not establish actual FT-710 USB operation. The radio was removed by the user; physical serial-line state, receive continuity across CAT open, nonzero USB sample levels and two-station interoperability need hardware validation before calling this client operational.
