# Task 108: Waveform Fill and Envelope Colours Do Not Match Reference

## Description
The waveform display fill colours and output envelope line are wrong compared to the Pro-L 2 reference.

**Issue 1 — Input/output waveform fills are nearly invisible:**
`MLIMColours::inputWaveform` is `0x70202840` (dark navy at ~44% alpha) and `outputWaveform` is `0x60182848` (even darker navy). Blended over the dark background gradient, both produce a result that is essentially black — the waveform fills are invisible when audio plays.

Reference frames (`v1-0005.png`, `v1-0006.png`) clearly show the waveform area filled with a **medium blue-grey** when signal is present — approximately `#6878A0` at 65% opacity, covering most of the display. When the input signal is at near-0 dBFS, the fill should span the full height, making the blue-grey color the dominant visual.

Correct values (from pixel-sampling the reference):
- `inputWaveform`:  `0xA8607898` — medium blue-purple at ~66% alpha
- `outputWaveform`: `0x804060A0` — slightly deeper blue at ~50% alpha (draws on top of input fill, so output level shows as a lighter overlay)

**Issue 2 — Output envelope line is amber/tan instead of white:**
`MLIMColours::outputEnvelope` is `0x80B89040` (amber). The reference (`v1-0005.png`, `v1-0006.png`) shows the output envelope as a **white/cream curved line** (`~#E0E8FF` at ~80% alpha). This line traces the output level and should be clearly visible against the blue-grey fill.

Correct value:
- `outputEnvelope`: `0xCCDDE8FF` — near-white with a slight blue tint, ~80% alpha

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/ui/Colours.h` — update `inputWaveform`, `outputWaveform`, `outputEnvelope` constants
Read: `/reference-docs/video-frames/v1-0005.png` — waveform colours clearly visible
Read: `/reference-docs/video-frames/v1-0006.png` — output envelope line is white

## Acceptance Criteria
- [ ] Run: `grep 'inputWaveform\|outputWaveform\|outputEnvelope' M-LIM/src/ui/Colours.h` → Expected: `inputWaveform` alpha byte > `0x90`, RGB component B > R (blue-dominant); `outputEnvelope` RGB all > 0xC0 (light/white)
- [ ] Run: `cmake --build build -j$(nproc) 2>&1 | tail -3` → Expected: build succeeds with exit 0

## Tests
None

## Technical Details
All three constants are in `M-LIM/src/ui/Colours.h`. Only edit the three colour values; no other code changes needed.

The layering order in `WaveformDisplay::paint()` is:
1. Background gradient (dark, shows when no signal)
2. Output fill (outputWaveform — lighter blue overlay)
3. Input fill (inputWaveform — main blue-grey fill)
4. Gain reduction (dark near-black fill — from top; see task 115 which also fixes this to be dark)
5. Output envelope (outputEnvelope — white line)

So `inputWaveform` is drawn ON TOP of `outputWaveform`. When input level is high, the input fill covers most of the display with the blue-grey colour. The output fill (lighter) shows below the input level, representing the output being lower than the input. This layering means `inputWaveform` should be slightly MORE opaque than `outputWaveform`.

## Dependencies
None
