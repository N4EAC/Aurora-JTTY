# Next debugging session

User will test later; do not start hardware testing or create a reminder automatically.

Production debug log:
`/Users/eduardo/Library/Application Support/JTTY Workbench/Logs/diagnostics.log`

Rotated preceding log:
`/Users/eduardo/Library/Application Support/JTTY Workbench/Logs/diagnostics.log.previous`

Current diagnostic build commit: fee7455. App: `build/JTTY Workbench.app`.

After the user confirms testing is complete, read these files directly. Correlate CAT connection request/open/polls with Qt helper selected device, format/state messages, CoreAudio device-loss error, termination and converted sample levels. Timestamps are UTC with milliseconds; convert to America/New_York for user-facing discussion.

Confirmed system-log evidence: USB CoreAudio device 125 was reported dead immediately before AUHAL fell back to built-in microphone 118. This identifies the immediate trigger, not why the device became unavailable. Standard Mic Mode was confirmed by the user. Do not claim the hardware issue is fixed based on simulated tests.

User's recommended fallback if this fails again: rebuild from WSJT-X itself, retaining JTTY and removing other modes, instead of continuing to patch the independent SwiftUI client. The current app reuses upstream receive audio but remains a separate frontend; the suggested fallback is a substantially fuller WSJT-X-based rebuild. Use the pinned GPL source and preserve licensing notices.
