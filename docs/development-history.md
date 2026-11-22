# Development history (internal builds, not public releases)

The first public USB DeClick version is **v0.1.0-beta.1**. Pre-release versions were engineering experiments and were not distributed as public GitHub releases.

| Internal build | Purpose / outcome |
| --- | --- |
| v0.3–v0.8 | USB gadget settings / SD autorun experiments. Several changes did not apply or targeted the wrong stream; none is a confirmed audio fix. |
| v0.9 | First ALSA `LD_PRELOAD` PCM interposer and phase-based correction, incomplete logging. |
| v0.9.1 | Proven PCM capture interception and phase tracking; residual clicks and restarts remained. |
| v0.9.2 | Strongest controlled hardware result: 25 vs 1,335 strong spike candidates (~98.1% reduction). |
| v0.9.3 | Conservative gating introduced; less effective in the tester's recordings. |
| v0.9.4 | Reuses v0.9.2 DSP core, adds manual ON/OFF and leaner diagnostics; tester reports comparable subjective quality. |
| v0.1.0-beta.1 | **Same v0.9.4 ARM64 binary and runtime scripts**, relabeled and documented for first public beta; checksum updated for changed autorun label. |

### Open questions

- Which component is responsible for the original 10-ms click corruption remains unknown.
- More click reduction is possible, but ON may alter clean musical transients.
- Proper CPU/RAM/latency measurements on MPC hardware and compatibility tests with other USB sources have not yet been conducted.
- SampQOL UI integration is a future goal; this project is independent and not endorsed by SampQOL or Akai.
