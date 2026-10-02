#!/bin/sh
set -eu
TASK_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
cd "$TASK_ROOT"
ctest --test-dir build --output-on-failure
python3 -m unittest discover -s tests -v
if [ "$(uname -s)" = Darwin ]; then
  swiftc -O -module-cache-path build/swift-module-cache -import-objc-header native/Bridge.h -L build -ljtty_live -Xlinker -rpath -Xlinker "$TASK_ROOT/build" native/Audio.swift tests/AudioHarness.swift -o build/test-audio
  swiftc -O -module-cache-path build/swift-module-cache -import-objc-header native/Bridge.h -L build -ljtty_live -Xlinker -rpath -Xlinker "$TASK_ROOT/build" native/Audio.swift native/Station.swift tests/StationHarness.swift -o build/test-station
  swiftc -O -module-cache-path build/swift-module-cache -import-objc-header native/Bridge.h -L build -ljtty_live -Xlinker -rpath -Xlinker "$TASK_ROOT/build" native/Audio.swift native/Station.swift tests/ReceiveCATHarness.swift -o build/test-receive-cat
  build/test-audio
  build/test-receive-cat
  # Optional device-output checks: built-in speakers and Hamlib dummy only.
  if [ "${JTTY_DEVICE_SMOKE:-0}" = 1 ]; then
    build/test-audio --silent-output
    build/test-station
  fi
fi
