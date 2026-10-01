# JTTY licensing and independent-client assessment

Checked October 1, 2026. Scope: JTTY, the amateur-radio keyboard-to-keyboard mode in WSJT-X, rather than unrelated similarly named terminal programs.

## Finding

Yes: an independently maintained JTTY client may reuse or adapt the published WSJT-X implementation under the GNU GPL version 3. No separate author permission is required for uses covered by that license. A distributed client derived from this code must satisfy GPL conditions; the permission is not an unrestricted authorization for a closed-source derivative.

The official development page explicitly permits modification under GPLv3 and says use outside that license requires an alternative arrangement. This is a source-based licensing assessment, not a legal opinion or a patent/trademark clearance.

## Verified source

- Official repository: https://github.com/WSJTX/wsjtx
- Release: https://github.com/WSJTX/wsjtx/releases/tag/v3.2.0-rc1
- Downloaded commit: `567ad29ce6abf3d4a44f181cdbc7ceba0d73e5f4`
- Local checkout: `../upstream/wsjtx`, detached at the release tag.
- Default `master` at retrieval was `967c85a6119110788b0df3782cf796485b2cee14` and contained no JTTY implementation. The release tag does contain it. Use the tag for reproducibility.
- The local release notes explicitly introduce JTTY. The official developer page links this repository: https://wsjtx.github.io/wsjtx/devel.html

## License evidence

`COPYING` supplies GPLv3. The repository README identifies GPLv3. `doc/common/license.adoc` additionally permits version 3 or any later version; preserve file-specific notices and verify the version option for components actually reused. GPLv3 is the baseline for the proposed client.

JTTY-specific code and documentation are in `lib/jtty`, with GUI, audio, and tests elsewhere. The inspected JTTY files do not provide a general exception permitting proprietary derivatives. The included `lib/jtty/portaudio.h` has its own permissive notice; third-party components require their own license review. This is not a complete dependency audit.

## What the permission means

| Approach | Assessment |
| --- | --- |
| Standalone application using or translating JTTY encoder/decoder code | Permitted under GPL; translating Fortran into another language does not eliminate derivative-work obligations. |
| Distributed fork or combined application linking the GPL implementation | Plan to license the covered combined work under GPLv3 and supply corresponding source. Static versus dynamic linking is not a reliable way to avoid copyleft. |
| Private development and use | GPL section 2 permits making and running private modified versions without a public-source release. |
| Selling a GPL client | Permitted; recipients retain GPL rights. |
| Separately written compatible implementation | May be independently licensed if it is truly independent and does not copy/adapt protected code or documentation. This repository does not establish blanket clearance for all protocol implementations or patents. |
| Separate UI communicating with WSJT-X over an external interface | Potentially a separate work, depending on actual coupling and what is copied. Merely putting GPL code in another process does not settle the issue. Interface coverage for JTTY still needs technical verification. |

These conclusions follow from GPL sections 0, 2, 4–6 and the distinction between a covered combined work and separate independent works. For a proprietary implementation, resolve the architecture and rights before incorporating upstream code; the official team offers an alternative-license route.

## Distribution obligations for a code-derived client

1. Preserve applicable copyright, license, and warranty notices and include GPLv3.
2. Mark changes prominently and date them; identify your own authorship without removing upstream attribution.
3. License the covered work as a whole under GPLv3 and avoid incompatible added restrictions.
4. With binaries, provide corresponding source using a compliant GPL section 6 method. Include required build scripts and interface files; a link to unmodified upstream alone is insufficient for a modified client.
5. Preserve/display appropriate legal notices where GPL section 5(d) applies. The upstream documentation also requests prominent attribution; its exact notice is preserved in `WSJTX-license.adoc`.
6. Review dependencies and bundled data/assets separately. Where applicable, GPLv3 requires installation information for covered User Products.

Sources: pinned license https://github.com/WSJTX/wsjtx/blob/567ad29ce6abf3d4a44f181cdbc7ceba0d73e5f4/COPYING and documentation notice https://github.com/WSJTX/wsjtx/blob/567ad29ce6abf3d4a44f181cdbc7ceba0d73e5f4/doc/common/license.adoc .

## Naming and endorsement

The published trademark policy separately covers WSJT, WSJT-X, MAP65, QMAP and their program icons. It expressly allows GPL forks, requires distinct branding for functional modifications, and allows factual attribution such as “based on WSJT-X.” Give an independent client its own product name and icon and identify it as unofficial.

JTTY is not listed among the marks in that policy. This absence is not proof that the name is unprotected. Describing support for the JTTY mode is different from suggesting official endorsement. No exhaustive trademark search was performed.

Policy: https://github.com/WSJTX/wsjtx/blob/567ad29ce6abf3d4a44f181cdbc7ceba0d73e5f4/TRADEMARK.md .

The documentation asks users of source to notify the team as a courtesy. That wording does not require advance approval for GPL-compliant reuse. No message was sent to the maintainers.

## Practical direction

For an independent open-source client, use GPLv3 for the covered application, reuse the pinned codec/decoder with notices intact, choose distinct branding, and test interoperability against the release fixtures. A proprietary code-derived client requires an alternative license from the relevant rights holders; an independently implemented client needs a separate assessment of what materials and components it uses.
