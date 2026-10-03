#!/usr/bin/env bash
set -euo pipefail
export PYTHONUTF8=1
cd "$(dirname "$0")/.."
python scripts/prepare-wsjtx-jtty.py
TASK_PREFIX="$(pwd)/build/hamlib-prefix"
export PATH="$TASK_PREFIX/bin:$PATH"
cmake -G Ninja -S build/wsjtx-jtty-source -B build/windows-build \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_POLICY_DEFAULT_CMP0167=NEW -DCMAKE_PREFIX_PATH="$TASK_PREFIX;/mingw64" \
  -DCMAKE_Fortran_COMPILER=gfortran \
  -DCMAKE_Fortran_FLAGS="-fallow-argument-mismatch -std=legacy" \
  -DHamlib_INCLUDE_DIR="$TASK_PREFIX/include" \
  -DHamlib_LIBRARY="$TASK_PREFIX/lib/libhamlib.dll.a" \
  -DWSJT_GENERATE_DOCS=OFF -DWSJT_SKIP_MANPAGES=ON -DWSJT_SKIP_QMAP=ON \
  -DWSJT_BUILD_UTILS=OFF -DWSJT_ENABLE_TESTS=OFF -DWSJT_ENABLE_GUI_SMOKE_TESTS=ON \
  -DWSJT_FORTRAN_LIBRARY_VARIANTS=OPENMP_ONLY \
  -DWSJT_FOX_OTP=ON -DWSJT_TRACE_CAT=ON -DWSJT_QDEBUG_TO_FILE=ON
cmake --build build/windows-build --target wsjtx -j 4
