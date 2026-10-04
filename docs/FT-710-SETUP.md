# Yaesu FT-710 setup

Follow [station setup](WSJTX-JTTY-SETUP.md), selecting Yaesu FT-710 in Radio settings. Match the CAT baud rate, stop bits and handshake to your radio configuration. Choose the radio's USB audio device for input and output, and Mono for a one-channel input.

Use DATA USB and the FT-710's USB modulation source for digital operation. Set USB audio output and modulation gain using the receive meter and the radio's ALC indication. Confirm the selected dial frequency before transmitting. CAT PTT and serial-line PTT require their matching radio configurations; consult the radio manual for your selected method.

The interface provides Monitor, Tune and Halt Tx. Tune/PTT operations key the radio. Begin with receive monitoring, then set appropriate power and modulation levels for transmission.

**Yaesu FT-710 on macOS:** In **Audio MIDI Setup**, select the radio’s **USB Audio Device** and set **Format to 44,100 Hz for both input and output**, instead of 48,000 Hz. This setting is required for the FT-710 to work with Aurora JTTY on macOS. Stop monitoring before changing the format, then reselect the USB input/output in **Radio / Audio** and restart Monitor.
