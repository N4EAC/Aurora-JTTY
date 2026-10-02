# JTTY audio diagnostics

The macOS build writes to `~/Library/Application Support/JTTY Workbench/Logs/wsjtx_syslog.log`. Rotated app logs remain in `logs/` beneath that directory. Startup records `JTTY deep audio diagnostics v1` and the Qt version.

On the first input failure of each capture attempt, an asynchronous collector saves the preceding 90 seconds of CoreAudio/audio-service events to `coreaudio-failure-YYYYMMDD-HHMMSS-mmm.log` in the same directory. Filename timestamps are UTC. The app log records the collector's filename and exit status; a nonzero exit means system-log collection failed. Automated fixtures skip this collector.

Trace records:

- `JTTY GUI`: settings, capture-start requests, monitor transitions and CAT online/PTT/frequency changes.
- `JTTY Qt capture`: preferred and requested PCM format, buffer settings, returned stream state/error and processed duration.
- `JTTY HAL`: property-change notifications, native device identity, UID, availability, transport, nominal rate, buffer size and input/output stream format. Failure snapshots enumerate devices so a replacement ID with the same UID can be distinguished from a missing device. Property query status codes accompany values; a failed query must not be interpreted as a real zero value.
- `JTTY PCM input`: incoming channel sample counts and RMS/peak dBFS once a second, before decoder gating. -160 dBFS denotes digital silence in these statistics. Levels are diagnostic observations; no audio recording or message payload is saved by this tracer.
- `JTTY configuration invalidating input`: the device handle removed after a reported capture error.

The Qt device name records requested selection, not independently verified effective routing. CoreAudio `running_any_client` can include other clients and is not proof that JTTY owns that stream. System-default input is also distinct from effective per-app routing.

For a comparison, fully quit the other radio applications, note whether CAT is disabled or connected, run Monitor briefly, then report whether sound was from the radio, from the microphone, or silent. Do not exercise Tune/PTT for a receive-only test. Correlate app timestamps with the corresponding `coreaudio-failure-*` file.

`coreaudio-probe.cpp` is a separate ten-second receive-only HAL diagnostic. It selects an exact device name, does not open serial ports or change hardware formats, clears every output buffer to silence, and stops on device failure. It remains independent of the client build.

For macOS devices with one native input channel, Mono/Left selections open a mono Qt stream; Right is rejected with a settings message. With automatic input buffering, capture preserves the device's existing buffer-frame count instead of accepting Qt's default hardware buffer resize. Explicit user buffer overrides remain effective.
