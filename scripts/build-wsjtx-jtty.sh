#!/bin/sh
set -eu
TASK_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
python3 "$TASK_ROOT/scripts/prepare-wsjtx-jtty.py"
cmake -S "$TASK_ROOT/build/wsjtx-jtty-source" -B "$TASK_ROOT/build/wsjtx-jtty-fork" \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_POLICY_DEFAULT_CMP0167=NEW \
  -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@5 \
  -DCMAKE_Fortran_COMPILER=/opt/homebrew/bin/gfortran \
  -DWSJT_GENERATE_DOCS=OFF -DWSJT_SKIP_MANPAGES=ON -DWSJT_SKIP_QMAP=ON \
  -DWSJT_BUILD_UTILS=ON -DWSJT_ENABLE_TESTS=ON -DWSJT_ENABLE_GUI_SMOKE_TESTS=ON \
  -DWSJT_FOX_OTP=ON -DWSJT_TRACE_CAT=ON -DWSJT_QDEBUG_TO_FILE=ON
cmake --build "$TASK_ROOT/build/wsjtx-jtty-fork" --target wsjtx jt9 -j 6
# Keep both builds available; the replacement is identifiable in Finder.
FORK_APP="$TASK_ROOT/build/Aurora JTTY.app"
rm -rf "$FORK_APP"
cp -R "$TASK_ROOT/build/wsjtx-jtty-fork/wsjtx.app" "$FORK_APP"
cp "$TASK_ROOT/build/wsjtx-jtty-fork/jt9" "$FORK_APP/Contents/MacOS/jt9"
cp "$TASK_ROOT/COPYING" "$FORK_APP/Contents/Resources/COPYING"
cp "$TASK_ROOT/THIRD_PARTY_NOTICES.md" "$FORK_APP/Contents/Resources/THIRD_PARTY_NOTICES.md"
touch "$FORK_APP"
printf '%s\n' "$FORK_APP"
