# JTTY fallback validation — 2026-10-02

Aurora JTTY restricts the final half-symbol observation fallback to its highest-ranked hypothesis. The primary coherent decoding ladder still searches the original four-candidate lists; CRC and source validation remain required. This is not an SNR cutoff or a callsign blacklist.

Field capture `261002_230600.wav` reproduced the isolated `DJ0EGE` result at 117.421 seconds, 1517.6 Hz, estimated −17 dB: 10/13 hard synchronization tones matched, with 27 hard symbol disagreements. Instrumented replay showed fourth-ranked acceptance on the half-symbol fallback, after the primary observations failed. That combination is consistent with a false acceptance, although a recording alone cannot establish the sender's intended message. The revised decoder emits no message from this capture.

The earlier capture `261002_181629.wav` retains `DE K6`, `DE K6LJ 59`, and `DE K6LJ 599 001`, at approximately 51.7, 53.6 and 55.5 seconds. These were first-ranked primary decodes with zero hard symbol disagreements.

A paired replay of 32 identical generated captures of `DE K6LJ 599 001` (eight per SNR) produced these completed-message counts:

| Simulated SNR | Before | After |
| --- | ---: | ---: |
| −12 dB | 8 | 8 |
| −15 dB | 7 | 7 |
| −18 dB | 0 | 0 |
| −21 dB | 0 | 0 |

Generation: `sjtty 'DE K6LJ 599 001' 1500 1 0 0 384 8 SNR`.
Replay: `rjtty 4.6 1 384 1500 20 FILES`.
Local recordings and replay outputs are retained in ignored `build/capture-analysis/`; original recordings are unchanged. They are not distributed with the source.

This modest sample does not establish sensitivity for every propagation condition. A real signal recoverable only as a lower-ranked half-symbol hypothesis may now be missed. Further field recordings should be checked against this baseline before tightening other thresholds.
