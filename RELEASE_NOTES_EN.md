# USB DeClick v0.1.0-beta.1 — First Public Beta

**Experimental USB click-reduction patch for Akai MPC Sample, tested with Google Pixel 8a (Android 16).**

USB DeClick reduces recurring, sharp digital clicks that can occur when sampling a phone's audio over USB. It runs on the MPC from an SD card, without an Android companion app or permanent firmware modification.

## What this beta includes

- **Real-time PCM click repair:** tracks a recurring 10 ms spike pattern and immediately corrects strong isolated stereo impulses using neighboring audio frames.
- **Manual ON/OFF control:** edit `autorun/USB_DECLICK_MODE.txt` before startup. `ON` enables repair; `OFF` leaves the MPC application unmodified at startup.
- **Targeted USB capture processing:** designed for the MPC's 44.1 kHz, stereo, 32-bit USB audio input.
- **Low-overhead design:** fixed-size audio state and no filesystem writes from the PCM processing loop. Actual long-term MPC CPU and RAM usage still need measurement.
- **Removable installation:** SD-card based, with a simple rollback by restarting the MPC without the mod card.
- **Monochrome startup screen** showing that the mod was launched.

## Test evidence

The issue was investigated on **Google Pixel 8a (Android 16) connected to Akai MPC Sample (firmware 1.3.0.12)**. Recorded audio revealed many isolated stereo sample spikes following a **441-frame / 10 ms pattern** at 44.1 kHz.

In a controlled comparison of approximately one-minute recordings of the same track at similar volume, the detector found:

| Without repair | With repair |
| ---: | ---: |
| **1,335** strong-spike candidates | **25** strong-spike candidates |

This corresponds to **approximately 98.1% fewer detected candidates** in that test. The comparison was performed with a development build using the **same audio repair core** as this release. The beta has been exercised on the MPC with qualitatively similar results, but a separate full benchmark is still outstanding.

The measurement does **not** mean all audible artifacts are removed, and results cannot be assumed to transfer to other devices. Read the [research findings](docs/research-findings.md) and [reproducible test methodology](docs/testing-method.md) for technical details.

## Device support

**Only the Pixel 8a was tested as an affected phone.** A Samsung Galaxy A06 and a laptop provided clean control recordings on the same MPC. Similar symptoms may exist on other phones, but **this release has not been tested on those models**. Reports from other devices are welcome; compatibility is not guaranteed.

## Installation and safety

When the **SD install ZIP** is available on [GitHub Releases](https://github.com/odeo0/usb-declick-mpc-sample/releases), download it and follow the [installation guide](INSTALL.md).

The repair is experimental. Some clicks can remain; **ON** may also alter genuine sharp musical transients. Use **OFF** when repair is not needed. There is **no on-screen toggle** yet: change the mode file and reboot. Do not merge this installation with other autorun modifications.

**Rollback:** power off, remove the mod SD card, and reboot. Internal firmware is not flashed.

## Help improve it

Please report your phone and MPC model, firmware, test conditions, ON/OFF setting, and ideally two recordings of the same passage. The source, analysis method, and [community report template](docs/community-test-report.md) are included for independent testing and further development.

**Unofficial experimental beta. Not affiliated with or endorsed by Akai.**
