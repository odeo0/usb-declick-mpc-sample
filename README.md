# USB DeClick

**Experimental USB audio click repair for Akai MPC Sample.**

USB DeClick is an unofficial, removable SD-card mod designed to reduce intermittent digital clicks when recording USB audio from a phone into an **Akai MPC Sample**. It operates on the MPC itself: the phone continues to use Android's normal USB audio output, so no special player or Android app is required.

> **First public beta:** `v0.1.0-beta.1` · **Tested affected device:** Google Pixel 8a (Android 16) → Akai MPC Sample (firmware 1.3.0.12).

## The problem

On the tested setup, USB recordings contained sharp clicks even when the signal was not clipping. Analysis of recorded PCM showed that many defects were isolated, simultaneous spikes in both stereo channels. Their positions followed a repeating pattern of **441 audio frames at 44.1 kHz — exactly 10 ms**. The position within that period could shift during a recording, and clicks sometimes returned after changes in playback or USB stream state.

The same MPC recorded clean audio from a **Samsung Galaxy A06 and a laptop** during control tests. This indicates a host-dependent USB audio interoperability issue, but does **not** establish whether the underlying cause is in Android, the USB transport, or the MPC audio pipeline.

Some users may experience similar symptoms with other phones. **This project has not tested affected phone models other than the Pixel 8a.** Compatibility and effectiveness on other devices remain unknown.

## How it works

The mod attaches to the MPC's USB audio capture path through a small ARM64 ALSA interception library. It targets the `UAC2Gadget` capture stream when it is **44.1 kHz, stereo, S32_LE**.

The repair combines two techniques:

1. **Phase tracking:** identify and follow the recurring position of suspicious samples within the 10 ms pattern, even as that position drifts.
2. **Immediate spike repair:** recognize sufficiently strong, isolated stereo spikes without waiting for the tracker to lock.

When the repair triggers, the affected audio frame is replaced by an estimate derived from its neighboring frames. The algorithm uses fixed-size state and does not write files from the PCM processing loop.

**This is a corrective workaround, not a fix for the underlying USB issue.** With repair **ON**, genuine sharp transients can sometimes be mistaken for clicks and altered. With repair **OFF**, the PCM processing is bypassed.

## Measured result

A controlled comparison used two approximately one-minute recordings of the same track at similar levels. Both recordings were analyzed with the same detector for strong, isolated, dual-channel sample spikes.

| Recording condition | Detected spike candidates |
| --- | ---: |
| Without USB DeClick | **1,335** |
| With the repair algorithm | **25** |

That is **approximately 98.1% fewer detected strong-spike candidates** in this comparison. The test used a development build with the **same DSP processing core** as this beta. The current beta has also been tested on the MPC with qualitatively similar results, but has **not** undergone a separate, fully controlled benchmark or long-term reliability test.

These counts are algorithmic measurements, not a guarantee of inaudible audio, complete restoration, or identical results across phones and recordings. Some clicks can remain. See the [research notes](docs/research-findings.md) and [test method](docs/testing-method.md) for details and reproducible analysis.

## Installation

**Requirements:** Akai MPC Sample with the tested firmware, an SD card supporting the MPC autorun mechanism, and a USB audio source.

1. Back up any important projects, switch off the MPC, and disconnect the USB source.
2. On a **test SD card**, remove any existing `autorun` directory. Do not merge this mod with another autorun package.
3. When the **SD install ZIP** is available on [GitHub Releases](https://github.com/odeo0/usb-declick-mpc-sample/releases), download it and copy its `autorun` folder to the root of the SD card.
4. Open `autorun/USB_DECLICK_MODE.txt` and set it to `ON` (repair) or `OFF` (no modification at startup). `ON` is the default in the beta package.
5. Insert the card and power on the MPC with the phone disconnected. If repair is enabled, the MPC application may restart once during initialization; wait until the normal interface is ready before connecting the phone.
6. Make a short test recording before using the mod for important work.

**To change modes:** switch off the MPC, edit the text file, and start it again. The switch is **not yet available on the MPC recording screen**.

**To uninstall or recover:** power off completely, remove the mod SD card, and boot normally. USB DeClick does **not** flash or permanently replace internal firmware.

See the [installation guide](INSTALL.md) for mode switching, diagnostics, and a recommended recording test.

## Compatibility and limitations

- **Tested as affected:** Pixel 8a, Android 16 → MPC Sample, firmware 1.3.0.12. Other affected phone models and firmware versions are **unverified**.
- **Clean controls:** Samsung Galaxy A06 and a laptop produced clean recordings on the same MPC. The patch was **not** evaluated for sound transparency on those clean sources.
- The mod checks the MPC application build before injecting its library; unrecognized builds are not supported.
- The algorithm is intentionally assertive when **ON**; it can modify legitimate audio transients. For clean sources, use **OFF**.
- Some artifacts may survive, especially when the USB stream restarts or the repeating spike position changes.
- No comprehensive on-device CPU, memory, thermal, or long-duration stability benchmark has been completed.
- Do not combine this package with other `autorun` or `LD_PRELOAD` modifications unless their launch mechanisms have been deliberately integrated.
- This is **unofficial experimental software**, not affiliated with or endorsed by Akai. Use at your own risk.

## Research, source code, and testing

The repository provides the [DSP source](src/pcmfix_core.c), [research findings](docs/research-findings.md), [testing methodology](docs/testing-method.md), and [device compatibility notes](docs/compatibility.md). Developers are welcome to reproduce the measurements, inspect the algorithm, and investigate the original USB audio problem.

The [v0.9.2 ALSA adapter source](src/reference-v092/pcmfix_alsa.c) is provided as a reference. The checked-in SD payload comes from v0.9.4. See [build provenance](BUILD_PROVENANCE.md) for details on source availability and verification.

On a host with a C compiler, Python 3, and a POSIX shell, run the core test and verify the checked-in payload with:

```sh
sh tools/test.sh
```

To report results, open a GitHub issue using the [test report template](docs/community-test-report.md). Include your phone model, Android version, MPC firmware, whether repair was ON or OFF, and comparable recording conditions. Please share audio files only when you have permission.

## License

Original USB DeClick source code is available under the MIT License. The patch includes no proprietary Akai application binary.
