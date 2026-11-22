#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
cc -O2 -Wall -Wextra -Isrc src/pcmfix_core.c tests/test_core.c -o /tmp/usb_declick_test_core
/tmp/usb_declick_test_core
python3 tools/verify.py
