# Test methodology and evidence levels

## Hardware and reproduction

- Host: Google Pixel 8a, Android 16, source audio from ordinary Android apps.
- USB gadget: Akai MPC Sample with firmware 1.3.0.12, USB Audio Class 2.0 capture at 44,100 Hz, two channels, 32-bit samples.
- Control: Samsung Galaxy A06 and a laptop did not present the same click problem when used with the same MPC.
- The recorded WAV files themselves are 44.1 kHz / stereo / PCM_24, distinct from the USB gadget's 32-bit interface.

## Strong one-frame stereo spike detector

For a normalized stereo signal x, per channel:

```
error[t,c] = abs(x[t,c] - 0.5*(x[t-1,c] + x[t+1,c]))
sharp[t,c] = abs(x[t,c]-x[t-1,c]) + abs(x[t,c]-x[t+1,c])
```

A frame is a candidate if **both** channel errors exceed 0.20 full-scale and both sharpness values exceed 0.40 full-scale. Adjacent candidates within two samples are grouped to prevent double-counting the same artifact. This is a **proxy**, not a perceptual noise metric; it may include real musical impulses or miss small audible clicks. Raw counts from older discussions used different criteria and should not be compared directly.

| File | Setup | Approx duration | Strong candidates (this detector) |
| --- | --- | ---: | ---: |
| sample 019 | Pixel, unpatched | 60.49 s | 1,335 |
| sample 017 | Pixel, v0.9.2 | 60.06 s | 25 |
| sample 020 | Pixel, v0.9.3 | 60.74 s | 41 |

The tracks and recording levels in 017 vs 019 were matched closely by the tester; do not treat 017 vs 020 as a controlled codec-level A/B test.

When the unchanged v0.9.2 core is run offline on raw PCM data from unpatched sample 019, the same detector reports about 23 remaining candidates; this is **an offline simulation**, not a field result for v0.9.4.

The correction algorithm also changes some predicted-phase samples where no spike is detectable. This may be audible or may suppress weaker errors, and needs clean-host testing.

## Current limits

We have not established whether corruption originates in Pixel Android USB host, UAC2 feedback negotiation, USB transfer handling, or post-capture MPC processing. The 10 ms periodicity is repeatable; assigning a kernel/software component as the cause would be premature.

CPU%, memory overhead, XRUN changes and stability on long recordings have not been measured on a real v0.9.4 system. New release candidates must be tested before promotion.