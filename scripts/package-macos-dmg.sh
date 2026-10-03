#!/bin/sh
set -eu
TASK_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$TASK_ROOT"
python3 scripts/stage-macos-dmg.py
mkdir -p installers
hdiutil create -volname "Aurora JTTY 1.1" -srcfolder build/dmg-stage -format UDZO -ov installers/Aurora-JTTY-1.1-macOS-arm64.dmg
hdiutil verify installers/Aurora-JTTY-1.1-macOS-arm64.dmg
shasum -a 256 installers/Aurora-JTTY-1.1-macOS-arm64.dmg > installers/Aurora-JTTY-1.1-macOS-arm64.dmg.sha256
