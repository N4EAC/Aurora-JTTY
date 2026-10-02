# Proposed JTTY layout — preview only

The current app is unchanged. Mockup: `jtty-layout-proposal.png`; editable source: `jtty-layout-proposal.svg` and its generator.

Top strip: existing band/preset selector and dial display, Rx/Tx offsets, frequency tolerance, both copy-frequency actions; mode/CAT status, radio/audio settings, Monitor, Halt Tx, Tune and Log contact.

Below: all received traffic at left; a larger frequency-scoped conversation transcript at right, with the existing draft field, Send, F1–F8 editable macros, contact/queued calls and serial number. The conversation can float into a dedicated window. It remains scoped to Rx ± tolerance, not silently filtered to one callsign.

Bottom: the existing waterfall and input meter in the main window, with device and CAT status. The mockup contains illustrative data; it is not a radio test or proof of received audio.

Implementation requirements:
- Reparent existing Qt widgets and preserve their object names/slot connections, ranges, validation and enable/disable rules. Keep upstream Radio/Audio/DSP/queue/PTT behavior unchanged.
- Dock/floating conversation must use one live set of transcript/composer/macros, not create another capture source, rig connection, decoder or send handler. Closing the floating pane restores it to the dock; it does not exit the app or stop monitoring unexpectedly.
- Main and floating layouts must resize without obscuring Halt Tx or frequency controls. Avoid duplicated shortcuts that enqueue a message twice.
- Preserve chronological received and outgoing history. Outgoing text must distinguish accepted/queued text from playback status; a TX label alone must not imply successful RF transmission.
- Clear actions must use the matching upstream history/cursor cleanup rather than clearing a QTextEdit that is re-rendered from retained history on the next decode.
- Show CAT, audio and TX status accurately; retain existing disconnected/busy/transmitting guards. Sample data belongs only to this proposal.
- Run existing JTTY receive and queued transmit loopback checks, startup checks, and verify frequency copying, tolerance, macros, docking, resize, settings, Stop/Halt Tx, tune and log behavior before delivering a modified app. Physical USB/CAT/PTT validation remains a separate hardware check.
