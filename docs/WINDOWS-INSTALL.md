# Aurora JTTY 1.0 — Experimental Windows x64

**This Windows version is experimental.** Installation and software startup tests passed on the build runner; physical radio USB audio and CAT/PTT still require validation on Windows 10/11 stations.

Run Aurora-JTTY-1.0-windows-x64-setup.exe to install for your Windows account. The portable ZIP can be extracted into a writable folder; open bin/Aurora-JTTY.exe. Keep the included DLLs, plugins and resource folders together. This first Windows build is unsigned.

Open Radio / Audio and enter your callsign/grid. Choose your Hamlib radio model, Windows COM port and serial settings matching your radio, then select CAT PTT. Choose the radio USB input and output independently. Test CAT and begin with Monitor. Tune/PTT operations transmit; use them only when intended.

The Windows build uses Hamlib directly and does not require OmniRig. App logs are stored beneath the Windows application-data directory in Logs. Software startup testing does not replace testing your radio, USB audio and CAT/PTT on the target machine.

COPYING and licenses/ contain license notices and dependency-source information. Complete application/Hamlib source and exact MSYS2 dependency source packages accompany the build artifact. Binary distributions must retain corresponding source access and notices.
