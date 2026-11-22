# Research notes: recurring USB audio clicks on Pixel 8a → MPC Sample

> **Scope of evidence:** These are observations from one tester's setup, **Google Pixel 8a (Android 16) connected to Akai MPC Sample (firmware 1.3.0.12)**. We have **not tested any other Pixel model or other reportedly affected phone model with USB DeClick**. Similar problems may occur on other Android phones, and anecdotal reports exist, but we cannot independently verify their frequency, underlying cause, or whether this patch helps. The Samsung Galaxy A06 and a laptop were used only as *clean control hosts* with the same MPC, not as validation of the patch on those hosts.

This document is provided so other users, firmware developers and the SampQOL community can independently reproduce the measurements, challenge the interpretation, and experiment with improvements. It is not a confirmed diagnosis of an Android or MPC kernel bug.

## 1. Hardware and symptom

- **Problematic tested host:** Google Pixel 8a, Android 16; ordinary Android apps playing audio over USB-C, no specialized Android USB player.
- **Recorder / USB gadget:** Akai MPC Sample, firmware 1.3.0.12, USB Audio Class 2.0 at 44,100 Hz, stereo, 32-bit ALSA PCM.
- **Recorded files:** 44,100 Hz stereo, 24-bit PCM WAV exported from MPC. This differs from the 32-bit USB interface representation.
- **Observed symptom:** sharp, intermittent digital-sounding clicks in recorded samples, sometimes changing frequency after Play/Pause and USB stream restarts.
- **Clean comparisons:** Samsung Galaxy A06 and a laptop did not reproduce the same audible crackle on the same MPC in the tester's earlier trials. This does **not** prove which device contains the defect.

Other hosts, cables, Android builds, firmware versions and audio applications have **not been characterized systematically**. We make no compatibility claims for them.

## 2. The strongest observation: 441-frame recurrence

At 44,100 frames/s, **441 frames = 10 ms**. Affected WAVs exhibit repeated sharp anomalies at nearly the same position modulo 441 frames. The position differs across recording starts and can drift or reset when the stream is restarted.

For example, the controlled unpatched recording `sample 019.wav` contained **1,335** strong spike candidates under the detector below. **1,293** of them fell in only two adjacent phases (365 and 366 modulo 441; 430 and 863 candidates, respectively). In the other unpatched-like recording `sample 023.wav`, **1,475 of 1,514** candidates clustered at phases 257 and 258. The latter file's ON/OFF mode was not explicitly labeled by the tester, so do not count it as an independent controlled baseline.

Concentration on a 10 ms grid suggests a block-processing/clock/transfer interaction, but **does not identify a responsible driver**. The Pixel's 48 kHz application mixer, 44.1 kHz gadget interface and USB Audio feedback behavior are candidate areas for investigation, not demonstrated root causes. Earlier AOC logs also contained `HF overrun` / `AMixUSB Input 1 RB overflow` around some playback transitions; those messages were **not synchronized to individual recorded clicks**.

## 3. Reproducible detector

The public `tools/analyze_wav.py` uses normalized stereo samples. For each frame `t`, both channels `c`:

```text
midpoint[t,c] = (x[t-1,c] + x[t+1,c]) / 2
error[t,c]    = abs(x[t,c] - midpoint[t,c])
sharp[t,c]    = abs(x[t,c]-x[t-1,c]) + abs(x[t,c]-x[t+1,c])
```

Count a candidate when **both** channels have `error > 0.20` and `sharp > 0.40` (normalized full scale); group candidates separated by 2 samples or fewer. This metric intentionally targets *strong one-frame stereo anomalies*, **not all audible defects**. It can miss quiet clicks and can mistake some real transients for clicks.

Run on your own lawful recordings:

```bash
python3 -m pip install numpy soundfile
python3 tools/analyze_wav.py '/path/to/sample ON.wav' '/path/to/sample OFF.wav'
```

| File | Test status | Duration | Strong candidates | Notes |
| --- | --- | ---: | ---: | --- |
| `sample 019` | **No patch**, Pixel 8a | 60.49 s | 1,335 | Controlled comparison |
| `sample 017` | Internal **v0.9.2**, Pixel 8a | 60.06 s | 25 | Same track, similar level |
| `sample 020` | Internal v0.9.3 | 60.74 s | 41 | Not a paired A/B trial |
| `sample 021` | Internal v0.9.4 testing, ON/OFF **unconfirmed** | 61.47 s | 19 | Exclude from claimed improvement |
| `sample 023` | Internal v0.9.4 testing, ON/OFF **unconfirmed** | 61.23 s | 1,514 | Exclude from claimed improvement |

For `017` vs `019`, the observed reduction in **raw candidate count** is `(1335 - 25)/1335 = 98.13%` (approximately **98.1%**). Rates normalized to the slightly different recording durations give almost the same result. This is a result for an **earlier experimental build (v0.9.2) on one host**, not a guaranteed performance figure for public `v0.1.0-beta.1`. Listening reports indicate a large improvement with some remaining intermittent clicks. The current beta uses the same v0.9.2 DSP core via internal v0.9.4, but its on-device performance has not been measured in the same controlled A/B configuration.

The original copyrighted music and the test WAV recordings are **not redistributed** in this repository.

## 4. How USB DeClick attempts to repair audio

USB DeClick does **not** repair USB packets or change clock negotiation. It mitigates the recorded symptom by transforming samples already delivered to the MPC userspace capture path:

1. An SD `autorun` loader confirms the known MPC application build and starts the original program with an ARM64 `LD_PRELOAD` shim.
2. The shim wraps ALSA `snd_pcm_readi` / `snd_pcm_readn` and applies processing **only** to the `UAC2Gadget` capture stream when it is 44.1 kHz, stereo, S32_LE and the user enabled repair.
3. The DSP examines a frame using its neighbors and maintains a **441-frame periodic phase**. It acquires a phase lock after enough strong aligned anomalies and tracks small shifts over time.
4. While phase-locked it **proactively interpolates** the predicted phase even if that frame does not individually exceed the strong-impulse detector. Separately, it can immediately interpolate very strong, same-direction stereo impulses before phase lock.
5. An affected frame is replaced by an interpolated value, approximately the midpoint of the preceding and following frame in each channel. The core requires **one-frame lookahead** (about 22.7 microseconds at 44.1 kHz). State is reset when ALSA capture is prepared again.
6. `OFF` before startup skips injection; when a loaded instance is bypassed, samples pass through unchanged. The documented and safest toggle is **edit the mode file and reboot between recordings**.

The DSP implementation is in [`src/pcmfix_core.c`](../src/pcmfix_core.c). The algorithm uses fixed-size state; it does **not** accumulate the full recording in memory. Optional SD diagnostics are written by a separate worker rather than the PCM callback. **Device CPU, RAM, XRUN and thermal impact remain unmeasured**.

### Why automatic phase tracking still exists in explicit ON mode

The **user** explicitly chooses ON/OFF; phase detection within ON is only for locating which samples to fix. The periodic artifact phase can move and reset, so using a fixed hard-coded frame position does not work reliably across recordings.

### Clean audio is not guaranteed unchanged in ON mode

Because predictive correction is aggressive, real transients could be replaced even on an initially clean host. Do **not** enable it by default for users without clicks until a clean-signal transparency test exists. Use OFF for unaffected devices. This tradeoff is the principal research limitation of the current DSP core.

## 5. What experiments did **not** establish

Early internal builds v0.3–v0.8 explored USB gadget request counts, sample rates and high-speed intervals. Several failed to apply; some targeted the **opposite USB stream direction**. The 48 kHz configuration failed the MPC capture-stream confirmation and was rolled back. They provide **no proof of a successful USB-protocol-level fix**. In particular, nothing in this project establishes a universal defect in Pixel firmware or a proven AOC resampling bug.

The later ALSA-interposition build v0.9.1 showed actual capture read interception, phase locking and corrections in the logs. Internal v0.9.2 improved click suppression; the more conservative v0.9.3 left more detected events; v0.9.4 returned to v0.9.2 processing and added explicit mode control, monochrome splash and lightweight RAM-only audio-path telemetry.

## 6. How to contribute useful new evidence

The most useful independent tests are **paired 60-second recordings of the same track at the same host volume**, with `ON` then `OFF`, rebooting the MPC between modes. Avoid pauses during the recordings; optionally run a separate Play/Pause/Play experiment with timestamps. Attach original WAVs only when legally shareable; otherwise attach detector output and brief logs.

Please report phone **model**, Android **version/build**, MPC **firmware**, USB connection/cable/adapters, source **app**, USB DeClick version/mode and audible result. For unaffected phones, **test OFF first** and be careful about possible changes to clean transients in ON. See [community test report](community-test-report.md).

## 7. Unknowns / follow-up experiments

- Does the same 10 ms artifact occur on other Pixel phones or other Android manufacturers? **Unverified.**
- Is corruption already present in ALSA capture or introduced elsewhere? The shim's success shows this point can suppress it, not the precise source.
- What happens to transient fidelity when ON with a clean USB host?
- What are actual CPU, RSS, XRUN, power and long-term stability costs on MPC?
- Can more selective repair retain v0.9.2-level click reduction without changing clean transients?
- Can a cooperative UI toggle be integrated into SampQOL without running conflicting autorun loaders?

**Project status:** experimental mitigator and shared research, **not** a universal audio driver fix or official Akai/SampQOL release.
