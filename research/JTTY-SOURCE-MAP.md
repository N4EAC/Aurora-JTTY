# JTTY source map

All paths below are relative to `upstream/wsjtx`, pinned to `v3.2.0-rc1`. JTTY is part of WSJT-X rather than a separate top-level repository.

| Area | Entry points |
| --- | --- |
| Mode design | `lib/jtty/jtty_design.md`, `jtty_design.txt`, `jtty_source_encoding.txt` |
| User behavior | `doc/user_guide/en/jtty.adoc` |
| Source packing | `lib/jtty/jtty_source_codec.f90` |
| FEC | `lib/jtty/jtty_fec_mod.f90`, `jtty_tbcc_code_profile.f90`, `jtty_tbcc_decoder.f90`, `jtty_tbcc_list_decoder.f90` |
| Encoding/waveform | `lib/jtty/genjtty.f90`, `gen_jttywave.f90`, `gen_syncwave.f90` |
| Receive/decoding | `lib/jtty/jtty_decode.f90`, `jtty_mdecode.f90`, `rjtty_sub.f90`, `jtty_mod.f90` |
| Standalone utilities | `lib/jtty/jtty.f90`, `sjtty.f90`, `rjtty.f90` |
| Streaming audio | `Modulator/JttyTxStream.cpp`, `Modulator/JttyPcmFifo.cpp` |
| UI | `widgets/mainwindow_jtty.cpp`, `widgets/JttyMessages.hpp` |
| Logger integration | `widgets/JttyN1mm.hpp`, `widgets/JttyN1mmOutput.hpp`, `lib/jtty/jtty_n1mm_integration.md` |
| Validation | `tests/unit/jtty/`, `tests/integration/jtty/`, `tests/fixtures/jtty/`, `samples/JTTY/` |
| Build integration | `CMakeLists.txt`, `CMake/Sources.cmake`, `CMake/Utils.cmake`, `tests/unit/jtty/CMakeLists.txt` |

The design specifies 4-GFSK, 1.888-second frames, 31.25 baud, about 127 Hz bandwidth, a 34-bit payload, 12-bit CRC, and rate-1/2 K=10 tail-biting convolutional coding. Messages can start without the fixed UTC cycles of FT8. Treat the pinned implementation and fixtures as the interoperability reference; this is a release candidate and may evolve.

A standalone client still needs audio I/O, radio/PTT control, receive-state handling, text queues, logging, and a UI around the codec. The map identifies starting points; it does not prove these files can be extracted without shared dependencies. The initial licensing research did not build the code. The subsequent prototype extracts a selected core with shared callsign and FFT routines; see `PROTOTYPE-STATUS.md` for its build and recorded-audio verification.
