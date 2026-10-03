# Aurora JTTY publication review

Reviewed 2026-10-02 against the pinned WSJT-X source and the current developer bundle.

Aurora JTTY may be distributed as a modified GPLv3-or-later application. It remains copyrighted; permission comes from its licenses. It cannot be released as a closed-source derivative with covered source withheld.

A release must:

- Include COPYING, original copyright notices, third-party notices and clear identification of modifications.
- Give binary recipients access to complete corresponding source under GPLv3 or later, including the pinned upstream checkout, overlays and necessary build scripts. A private GitHub repository alone is insufficient for public binary recipients. A complete source archive alongside the binary is one practical method.
- Preserve the independent Aurora JTTY identity and avoid claiming WSJT Development Group endorsement. The upstream TRADEMARK.md permits independently renamed forks; it does not establish clearance for the Aurora name in every jurisdiction.
- Fulfill the licenses of redistributed dependencies: FFTW GPLv2-or-later; Hamlib and libusb LGPLv2.1-or-later; Qt GPLv3/LGPLv3 as applicable; Boost Software License; GCC runtime exception and associated notices. For bundled LGPL libraries, preserve recipients' ability to replace or relink the libraries as required by the applicable license.

The current app is a local developer bundle linked to Homebrew libraries. It is not yet a self-contained, signed and notarized public macOS release. Review the final bundled libraries, plugins, licenses and source archive before publishing it. Signing and notarization are separate packaging tasks, not substitutes for license compliance.

This review supports publication under those conditions, not an unconditional guarantee of freedom from all copyright or trademark claims.

Sources: upstream/wsjtx/COPYING, upstream/wsjtx/doc/common/license.adoc, upstream/wsjtx/TRADEMARK.md, THIRD_PARTY_NOTICES.md, https://www.gnu.org/licenses/gpl-3.0.html and https://www.gnu.org/licenses/gpl-faq.en.html.

Upstream end-user documentation is separately licensed CC BY-ND 4.0 (see https://github.com/WSJTX/wsjtx/blob/567ad29ce6abf3d4a44f181cdbc7ceba0d73e5f4/DOCS-LICENSE.md). Redistribute those guides unmodified with attribution and links to the originals/license, or write independent fork documentation; do not publish adapted versions without permission. Original guide: https://wsjtx.github.io/wsjtx/guide.html . License: https://creativecommons.org/licenses/by-nd/4.0/ .
