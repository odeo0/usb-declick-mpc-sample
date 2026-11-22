#!/bin/sh
# v0.9.4 — stock app launcher + optional bounded SD diagnostics.
# During PCM capture, the ALSA library makes no file system syscalls.
set -u
DIR="$1"
LOG="$DIR/usb_declick_v094.log"
RT="$DIR/pcmfix_runtime_v094.log"
APP='/usr/bin/MPC Sample'
LIB='/run/mpc-pcmfix/libmpc_pcmfix.so'
STATUS='/run/mpc-pcmfix/status_v094.txt'
CONTROL='/run/mpc-pcmfix/control_v094.txt'
FLAG='/run/mpc-pcmfix/initialized_v094'
STATE='/run/mpc-pcmfix/runner.started'
report(){ printf '%s\n' "$*" >> "$LOG" 2>/dev/null || :; }
[ -e "$STATE" ] && { report 'V094_DUPLICATE_SKIPPED'; exit 0; }
: > "$STATE" || exit 0
report "V094_RUNNER_STARTED $(date -u 2>/dev/null)"
[ -f "$DIR/ENABLE_DIAGNOSTICS" ] && DIAG=1 || DIAG=0
[ "$DIAG" -eq 1 ] && printf 'USB_DECLICK_V094_DIAGNOSTICS_START\n' > "$RT" 2>/dev/null || :
sleep 4
[ -f "$DIR/ENABLE_PCM_FIX" ] || { report 'V094_DISARMED_BEFORE_LAUNCH'; exit 0; }
[ -f "$LIB" ] || { report 'V094_MISSING_LIBRARY'; exit 0; }
report 'V094_STOP_STOCK_APP'
if ! systemctl stop ac50.service; then report 'V094_STOP_FAILED'; exit 0; fi
if [ -f "$DIR/pcmfix-splash.raw" ] && [ -c /dev/fb0 ]; then
    echo 1 > /sys/class/graphics/fb0/blank 2>/dev/null || :
    echo 0 > /sys/class/graphics/fb0/blank 2>/dev/null || :
    if cat "$DIR/pcmfix-splash.raw" >/dev/fb0 2>/dev/null; then report 'V094_MONO_SPLASH_WRITTEN'; fi
fi
sleep 2
rm -f "$FLAG" "$STATUS" "$CONTROL" 2>/dev/null || :
report 'V094_START_APP_WITH_PRELOAD'
(
    ulimit -S -s 1024
    export MALLOC_ARENA_MAX=1
    export LD_PRELOAD="$LIB"
    exec setarch -R -- "$APP"
) >/dev/null 2>&1 &
PID=$!
sleep 4
if ! kill -0 "$PID" 2>/dev/null; then
    report 'V094_APP_EXITED_EARLY'; systemctl start ac50.service || :; exit 0
fi
if ! grep -q 'libmpc_pcmfix.so' "/proc/$PID/maps" 2>/dev/null; then
    report 'V094_PRELOAD_NOT_MAPPED'; kill "$PID" 2>/dev/null; wait "$PID" 2>/dev/null || :;
    systemctl start ac50.service || :; exit 0
fi
report "V094_PRELOAD_MAPPED pid=$PID"
[ -f "$FLAG" ] || sleep 5
if [ ! -f "$FLAG" ]; then
    report 'V094_CONSTRUCTOR_UNCONFIRMED_ROLLBACK'; kill "$PID" 2>/dev/null; wait "$PID" 2>/dev/null || :;
    systemctl start ac50.service || :; exit 0
fi
report 'V094_CONSTRUCTOR_CONFIRMED'
report 'V094_READY: 44100Hz stereo S32LE UAC2Gadget only; v092 DSP; mode=ON'
prev=''
prev_mode=ON
while kill -0 "$PID" 2>/dev/null; do
    if [ -r "/proc/$PID/stat" ] && grep -q ') Z ' "/proc/$PID/stat" 2>/dev/null; then
        report 'V094_APP_ZOMBIE_DETECTED'; break
    fi
    # Runtime ON/OFF is polled outside the audio callback. OFF is a byte-perfect bypass.
    mode="$(tr -d '\r\n ' < "$DIR/USB_DECLICK_MODE.txt" 2>/dev/null)"
    [ -f "$DIR/ENABLE_PCM_FIX" ] || mode=OFF
    case "$mode" in ON|OFF) :;; *) mode=OFF;; esac
    if [ "$mode" != "$prev_mode" ] && [ -f "$CONTROL" ]; then
        if [ "$mode" = ON ]; then char=1; else char=0; fi
        printf '%s' "$char" | dd of="$CONTROL" bs=1 count=1 conv=notrunc 2>/dev/null || :
        report "V094_RUNTIME_MODE_CHANGED $prev_mode -> $mode"
        prev_mode="$mode"
    fi
    if [ "$DIAG" -eq 1 ] && [ -r "$STATUS" ]; then
        snapshot="$(head -n 1 "$STATUS" 2>/dev/null)"
        if [ -n "$snapshot" ] && [ "$snapshot" != "$prev" ]; then
            printf '%s %s\n' "$(date -u '+%H:%M:%S' 2>/dev/null)" "$snapshot" >> "$RT" 2>/dev/null || :
            prev="$snapshot"
        fi
        len="$(wc -c < "$RT" 2>/dev/null)"
        case "$len" in ''|*[!0-9]*) len=0;; esac
        if [ "$len" -gt 16000 ]; then
            tail -c 8000 "$RT" > "$RT.trim" 2>/dev/null && mv -f "$RT.trim" "$RT" || :
        fi
    fi
    sleep 5
done
wait "$PID" 2>/dev/null
CODE=$?
report "V094_APP_EXIT code=$CODE"
if [ "$(systemctl is-system-running 2>/dev/null)" != stopping ]; then
    systemctl start ac50.service && report 'V094_STOCK_RESTORED' || report 'V094_RESTORE_FAILED'
fi
exit 0
