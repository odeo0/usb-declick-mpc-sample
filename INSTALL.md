# USB DeClick v0.1.0-beta.1 installation guide

This first public beta candidate was renamed from internal build v0.9.4. A tester reported audio quality similar to v0.9.2. The v0.9.4 runtime library and processing mechanism were carried over unchanged. Long-term stability, CPU and RAM usage, and overall audio quality still need testing.

## Before installing

- Use a test SD card and back up important projects separately.
- Power off the MPC and disconnect the phone.
- Remove the entire previous `autorun` folder before copying files from the beta package.
- Do not combine USB DeClick with SampQOL. Both packages use their own mechanism to modify the MPC application.

## Install and select a mode

1. Extract the SD install ZIP and copy the `autorun` folder to the root of the SD card.
2. Open `autorun/USB_DECLICK_MODE.txt`. The package defaults to `ON`, which enables repair. To disable the mod, replace `ON` with `OFF` and save the file before booting. With `OFF`, the MPC application starts without modification.
3. Power on the MPC with the phone disconnected. In `ON` mode, the monochrome `USB DECLICK / ON` splash screen appears and the MPC application may restart once.
4. When the normal MPC interface appears, connect the phone and make a short test recording.

The MPC interface has no mode toggle. To switch modes, power off the MPC, edit the mode file on the SD card, and restart. The script also checks the mode file during operation. Live changes are intended for development, outside recordings and while no other device is writing to the SD card.

## Recovery and diagnostics

Power off the MPC completely, remove the SD card containing `autorun`, and boot without it. Installation makes no changes to internal firmware.

Include `autorun/usb_declick_v094.log` when reporting an issue. The logs retain the internal `V094` identifier for compatibility with the existing runtime library. For detailed diagnostics, create an empty file named `ENABLE_DIAGNOSTICS` inside `autorun` before booting. The `.example` file alone does not enable diagnostics. The runtime then writes a size-limited `autorun/pcmfix_runtime_v094.log`.

## Recommended test

Record 60 seconds of the same track from a Pixel 8a at a fixed level with `ON`. Power-cycle the MPC with `OFF` selected and record another 60 seconds at the same level. Avoid pausing or restarting playback during either recording. Save the WAV files and logs separately, then compare the recordings by ear and with the [test method](docs/testing-method.md). Repair can alter genuine musical transients. A comparison with the click-free Samsung Galaxy A06 is also needed before a stable release.

## Processing and resource use

The mod targets one USB ALSA capture stream at 44.1 kHz, stereo, S32_LE. Other inputs, USB descriptors, and sample rates are unaffected. Processing uses fixed-size state and performs no SD logging in the PCM loop. Resource use on the MPC has yet to be measured.
