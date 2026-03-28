#!/bin/bash
cd SystemMonitor/tests
make clean
make
for f in test_* benchmark_*; do
  if [ -x "$f" ] && [ ! -d "$f" ] && [[ "$f" != *.cpp ]]; then
    ./$f
  fi
done
