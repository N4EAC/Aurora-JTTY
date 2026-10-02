#!/bin/sh
set -eu
TASK_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
cmake --build "$TASK_ROOT/build/wsjtx-jtty-fork" --target wsjtx sjtty test_jtty_source_codec test_jtty_receive_state test_jtty_message_matching -j 6
QT_QPA_PLATFORM=offscreen ctest --test-dir "$TASK_ROOT/build/wsjtx-jtty-fork" --output-on-failure \
  -R '^(test_wsjtx_startup|test_wsjtx_live_audio_jtty|test_wsjtx_jtty_tx_loopback|test_jtty_source_codec|test_jtty_receive_state|test_jtty_message_matching)$'
