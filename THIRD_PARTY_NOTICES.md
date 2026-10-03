# Third-party notices

Aurora JTTY is an independent, unofficial application. Its new adapter, client and UI code are licensed under GNU GPL version 3 or later. The original Swift client links selected unmodified WSJT-X source; the replacement Qt client applies a reproducible JTTY-only source overlay to WSJT-X pinned at commit 567ad29ce6abf3d4a44f181cdbc7ceba0d73e5f4 (v3.2.0-rc1). Original files and notices remain in upstream/wsjtx. COPYING contains GPLv3; doc/common/license.adoc in upstream grants the later-version option.

Upstream attribution (verbatim from the documentation license notice):

*The algorithms, source code, look-and-feel of _{prog}_ and related
programs, and protocol specifications for the modes FSK441, FST4,
FST4W, FT4, FT8, JT4, JT6M, JT9, JT44, JT65, JTMS, Q65, QRA64, ISCAT,
and MSK144 are Copyright (C) 2001-2026 by one or more of the following
authors: Joseph Taylor, K1JT; Bill Somerville, G4WJS; Steven Franke,
K9AN; Nico Palermo, IV3NWV; Uwe Risse, DG2YCB; Brian Moran, N9ADG;
John Nelson, G4KLA; Charles Suckling, DL3WDG; Roger Rehr, W3SZ; 
Greg Beam, KI7MT; Michael Black, W9MDB; Edson Pereira, PY2SDR; Philip
Karn, KA9Q; and other members of the WSJT Development Group.*


JTTY was developed by Joe Taylor K1JT, Steve Franke K9AN and the WSJT Development Team. No endorsement is claimed. The upstream trademark policy is preserved in upstream/wsjtx/TRADEMARK.md.

FFTW (https://www.fftw.org/) is linked as a system-installed build dependency. FFTW is distributed under GPL version 2 or later, compatible with this GPLv3 application. GNU Fortran runtime libraries use their own GCC Runtime Library Exception. Python and Apple's frameworks are external runtime dependencies. These binaries are a local development build; libraries are not bundled into a redistributable package.

Before distributing a binary, provide the corresponding covered source and build scripts and satisfy dependency license requirements, including FFTW source obligations for any redistributed FFTW binary. The shallow upstream repository is part of the source needed to build this client; distributing only the new adapter files is insufficient.

Hamlib (https://hamlib.github.io/) is dynamically linked as an external Homebrew dependency. Its library is licensed under LGPL version 2.1 or later; original notices remain in its source and installed package. No Hamlib binary is bundled in this developer app. Redistributed libraries require their applicable source and notice obligations.

The receive helper compiles unmodified WSJT-X Audio/SoundInput, AudioDevice and AudioStreamDescriptor source from the pinned checkout, under the project's GPLv3-or-later terms. Its logging is connected to a small stderr facade instead of WSJT-X's Boost logger; no upstream file is edited. Qt 5 Core and Multimedia are external Homebrew dependencies available under GPLv3 or LGPLv3 (and commercial licenses). This GPLv3 application uses the open-source terms; Qt frameworks and plugins are not bundled in the developer app. Preserve applicable Qt notices and source/build obligations when redistributing its binaries.

The replacement Qt client builds WSJT-X MainWindow, Configuration, receive and transmit audio, messaging, and transceiver code directly. Its source overlay changes application identity/icon, restricts mode selection to JTTY, and adds diagnostics and a mode restriction smoke check. The reproducible overlay is scripts/prepare-wsjtx-jtty.py; the original source and copyright notices remain unchanged in the pinned upstream submodule. Boost is dynamically linked as an external Homebrew dependency under the Boost Software License 1.0.

libusb (https://libusb.info/) is an external dynamically linked dependency under LGPL version 2.1 or later. Preserve its license, notices and applicable source obligations if its binary is redistributed.
