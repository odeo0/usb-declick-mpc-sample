#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
clang --target=aarch64-linux-gnu -ffreestanding -fuse-ld=lld -shared -fPIC -O2 -nostdlib \
  -Wl,-soname,libmpc_pcmfix.so -Wl,-z,now -Wl,--version-script=src/alsa_symbols.map \
  -o autorun/libmpc_pcmfix.so src/pcmfix_alsa.c src/pcmfix_core.c
python3 tools/refresh_checksums.py
python3 tools/verify.py
