# Playback / DAC / AEC commissioning contract

Target: pinned XMOS I2S-master 1.0.8_48k, XIAO slave48k32bitstereo; USB OUT48k16bitstereo. Default output:3.5mm active speaker. This contract is an acceptance requirement, not a claim of completed hardware validation.

1. Record board revision, image file/commit/SHA256 used during XMOS maintenance, VERSION/BLD_MSG/USB_BIT_DEPTH and actual48k clocks. Version alone does not identify the variant. Do not replace the I2S image with a USB image simply because its version is newer.
2. Record OP_L=(6,3), input packing0, output packing(0,0), upsampling(1,1), and I2S_INACTIVE=0. Confirm32-bit slot alignment with captured PCM before gain tuning.
3. Verify the intended Host→XVF3800→DAC route against the target firmware/board documentation and controlled signal measurements. Do not infer it merely from GPIO44 or from audible output. Record the evidence/source and any far-end DSP settings actually used.
4. Establish DAC output state using the documented control owner for this variant. Record analog mute/gain state and repeatability after cold start. AIC3104 at0x18 is not an invitation to issue speculative host writes: the official route description distinguishes XVF-controlled and host-direct arrangements. Leave ownership unresolved until confirmed rather than introducing competing initialization.
5. With conservative speaker volume, play a known signal and record physical output, software mute and several volume settings. Volume is applied before XIAO transmits PCM; confirm reference matches this scaled signal. Microphone mute must not stop output.
6. Record AEC bypass=0 and finite nonzero reference gain, with convergence observations during actual playback. A readable flag, convergence=1, or audible output alone does not prove echo suppression. If bypass=1, zero reference, or unknown route is observed, AEC acceptance fails; do not automatically tune undocumented values.
7. Compare playback-only mic capture, near-end-only speech and double-talk under fixed gain/placement. Save raw recordings and report residual echo, clipped samples, wake false positives, near-end intelligibility and ChatGPT interruption/first-sentence behavior. Evaluate1–3m and case microphone orientation. Define numeric acoustic acceptance thresholds before declaring success.

Evidence table (fill on hardware):

| Evidence | Actual result |
| --- | --- |
| Board / XMOS variant and maintenance hash | Pending |
| Clocks / PCM alignment / route readback | Pending |
| Route source and physical signal evidence | Pending |
| DAC control owner / mute / analog gain / cold start | Pending |
| Output signal / volume / microphone mute independence | Pending |
| AEC bypass / reference gain / convergence observations | Pending |
| Playback-only / near-end / double-talk recordings | Pending |
| Acoustic thresholds and measured results | Pending |

Sources: [official signal paths and commands](https://wiki.seeedstudio.com/respeaker_xvf_3800_i2c_list/), [48k master configuration](https://wiki.seeedstudio.com/respeaker_xvf3800_xiao_home_assistant/), [volume example](https://wiki.seeedstudio.com/respeaker_xvf3800_xiao_volume/). The volume example is not proof that the same direct-DAC control belongs in the chosen routed design.
