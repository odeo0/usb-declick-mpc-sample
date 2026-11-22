#!/bin/sh
# USB DeClick public v0.1.0-beta.1 (internal build V094) — transient SD autorun, stock MPC Sample 1.3.0.12 only.
set -u
DIR="$(cd "$(dirname "$0")" && pwd)" || exit 0
LOG="$DIR/usb_declick_v094.log"
report(){ printf '%s\n' "$*" >> "$LOG" 2>/dev/null || :; }
: > "$LOG" 2>/dev/null || :
report "V094_AUTORUN $(date -u 2>/dev/null)"
[ -f "$DIR/ENABLE_PCM_FIX" ] || { report 'V094_OFF_NO_MARKER: stock app untouched'; exit 0; }
MODE="$(tr -d '\r\n ' < "$DIR/USB_DECLICK_MODE.txt" 2>/dev/null)"
[ "$MODE" = ON ] || { report "V094_OFF_MODE=$MODE: stock app untouched"; exit 0; }
STOCK='/usr/bin/MPC Sample'
EXPECTED_FW=44507c7a7d3029c411b2e3780ea286ca3e08b7e3ffd7b5047d52f73bf5299d1f
ACTUAL_FW="$(dd if="$STOCK" bs=4096 count=1 2>/dev/null | sha256sum | cut -d ' ' -f 1)"
[ "$ACTUAL_FW" = "$EXPECTED_FW" ] || { report "V094_UNRECOGNIZED_FIRMWARE actual=$ACTUAL_FW"; exit 0; }
expected='2e0eedf02c70d1458698cbdc894262e5c65a0c1fff8dd2ed9df7c30e2bd2eda3'
expected_runner='68b968598055166e184299e05ebcef04e6fb5eedf46868c40392aeb886df479f'
actual="$(sha256sum "$DIR/libmpc_pcmfix.so" 2>/dev/null | cut -d ' ' -f 1)"
[ "$actual" = "$expected" ] || { report "V094_LIBRARY_SHA256_FAIL actual=$actual"; exit 0; }
actual_runner="$(sha256sum "$DIR/pcmfix-run.sh" 2>/dev/null | cut -d ' ' -f 1)"
[ "$actual_runner" = "$expected_runner" ] || { report "V094_RUNNER_SHA256_FAIL actual=$actual_runner"; exit 0; }
mkdir -p /run/mpc-pcmfix || { report 'V094_NO_RUN_DIR'; exit 0; }
cp "$DIR/libmpc_pcmfix.so" /run/mpc-pcmfix/libmpc_pcmfix.so || { report 'V094_COPY_LIBRARY_FAILED'; exit 0; }
cp "$DIR/pcmfix-run.sh" /run/mpc-pcmfix/pcmfix-run.sh || { report 'V094_COPY_RUNNER_FAILED'; exit 0; }
chmod 755 /run/mpc-pcmfix/pcmfix-run.sh
report 'V094_PAYLOAD_VERIFIED_QUEUED'
( setsid /bin/sh /run/mpc-pcmfix/pcmfix-run.sh "$DIR" </dev/null >> "$LOG" 2>&1 & )
exit 0
