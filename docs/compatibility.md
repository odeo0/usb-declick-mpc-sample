# Compatibility and tested devices

**Only one affected phone and recorder combination has been field-tested:** **Google Pixel 8a (Android 16) → Akai MPC Sample (firmware 1.3.0.12)**. The tester has **no means of verifying the same issue on other phone models**. Some users may encounter similar symptoms on other Android phones, but those potential cases and compatibility with USB DeClick are **not established** here.

| USB source | Tested with MPC Sample | Observed outcome | USB DeClick verification |
| --- | --- | --- | --- |
| Google Pixel 8a, Android 16 | Yes | Recurring 10-ms-aligned clicks without patch | **Yes, earlier experimental builds**, large reduction with v0.9.2; v0.9.4 reported similar subjective result |
| Samsung Galaxy A06 | Yes, as control | No comparable audible clicks | **Not verified ON;** recommended OFF |
| Laptop USB audio host | Yes, as control | No comparable audible clicks | **Not verified ON;** recommended OFF |
| Other Pixel models | No | Unknown; similar symptoms may have been reported | **Unknown** |
| Other Android manufacturers/models | No systematic patch testing | Unknown | **Unknown** |

The current public version (`v0.1.0-beta.1`) is a repackaging of the internal v0.9.4 executable and retains the earlier v0.9.2 DSP. **It has not yet been independently validated on additional MPC units or phone models.** A clean recording from a different host is not proof of clean behavior when the aggressive DSP is enabled.

If you own another potentially affected phone, please follow [community test instructions](community-test-report.md) and open a GitHub issue. We welcome both successful and unsuccessful results; do not infer compatibility from the label “USB Audio Class 2.0” alone.