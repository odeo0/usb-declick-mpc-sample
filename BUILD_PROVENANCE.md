# Build provenance

- **Public version:** `v0.1.0-beta.1`.
- **Internal build:** `V094`, based on `v0.9.2` DSP core.
- The compiled ARM64 `autorun/libmpc_pcmfix.so` is **byte-identical** to the internal v0.9.4 build.
- `autorun/pcmfix-run.sh` is **byte-identical** to the internal v0.9.4 build.
- The displayed `autorun.json` version string and `autorun.sh` comment changed; `autorun.check` was regenerated as required by the MPC SD loader. No DSP logic was changed.
- Runtime logs retain `v094` suffix for traceability and binary compatibility.
- This is a beta candidate, not official firmware and not a verified fix for every USB host.

The checked-in `autorun/` directory was recovered from the SD beta ZIP and preserves its file bytes. Run `python3 tools/verify.py` on a host with a POSIX shell to check the payload.

The v0.9.2 archive contains the same DSP core and symbol map as this repository. Its ALSA adapter is archived in [src/reference-v092/pcmfix_alsa.c](src/reference-v092/pcmfix_alsa.c). The exact v0.9.4 adapter source is still missing. The full runtime rebuild in `tools/build.sh` requires that source at `src/pcmfix_alsa.c`.
