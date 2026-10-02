#!/bin/sh
set -eu
TASK_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
cmake -S "$TASK_ROOT" -B "$TASK_ROOT/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$TASK_ROOT/build" -j 4
if [ "$(uname -s)" = Darwin ]; then
  mkdir -p "$TASK_ROOT/build/JTTY Workbench.app/Contents/MacOS"
  swiftc -O -module-cache-path "$TASK_ROOT/build/swift-module-cache" -import-objc-header "$TASK_ROOT/native/Bridge.h" -L "$TASK_ROOT/build" -ljtty_live -Xlinker -rpath -Xlinker "$TASK_ROOT/build" "$TASK_ROOT/native/Audio.swift" "$TASK_ROOT/native/Station.swift" "$TASK_ROOT/native/LiveView.swift" "$TASK_ROOT/native/Workbench.swift" -o "$TASK_ROOT/build/JTTY Workbench.app/Contents/MacOS/JTTYWorkbench"
  mkdir -p "$TASK_ROOT/build/JTTY Workbench.app/Contents/Resources"
  cp "$TASK_ROOT/native/Assets/JTTY.icns" "$TASK_ROOT/build/JTTY Workbench.app/Contents/Resources/JTTY.icns"
  cp "$TASK_ROOT/build/jtty-audio-input" "$TASK_ROOT/build/JTTY Workbench.app/Contents/MacOS/jtty-audio-input"
  cp "$TASK_ROOT/native/Info.plist" "$TASK_ROOT/build/JTTY Workbench.app/Contents/Info.plist"
  touch "$TASK_ROOT/build/JTTY Workbench.app"
fi
