# Task 291: Add "GAIN" Static Label Below Input Gain Badge

## Description
The Pro-L 2 reference (visible in `v1-0009.png` bottom-left of waveform) shows a small "GAIN" text
label positioned below the input gain value badge (e.g. "+0.0"). M-LIM has the badge but is missing
this static "GAIN" label.

Add a small `juce::Label` with text "GAIN", styled in `textSecondary` colour and `kFontSizeSmall`
font, positioned directly below the `inputGainValueLabel_` badge.

## Produces
None

## Consumes
None

## Relevant Files
Modify: `M-LIM/src/PluginEditor.h` — add a `juce::Label gainLabel_` member
Modify: `M-LIM/src/PluginEditor.cpp` — initialise, style, and position `gainLabel_` below the badge
Read: `/reference-docs/video-frames/v1-0009.png` — shows "+0.0" badge with "GAIN" below it

## Acceptance Criteria
- [ ] Build: `cmake --build /workspace/M-LIM/build --config Release -j$(nproc) --target MLIM_Standalone` → Expected: exits 0
- [ ] Visual: Screenshot of waveform bottom-left shows badge (e.g. "+0.0") with small "GAIN" text
  label directly below it, matching the reference layout.

## Tests
None

## Technical Details
In `PluginEditor.h`, add:
```cpp
juce::Label gainLabel_;
```

In `PluginEditor.cpp` constructor, after `inputGainValueLabel_` setup:
```cpp
gainLabel_.setText("GAIN", juce::dontSendNotification);
gainLabel_.setFont(juce::Font(MLIMColours::kFontSizeSmall));
gainLabel_.setColour(juce::Label::textColourId, MLIMColours::textSecondary);
gainLabel_.setJustificationType(juce::Justification::centred);
gainLabel_.setInterceptsMouseClicks(false, false);
addAndMakeVisible(gainLabel_);
```

In `resized()`, position it below the badge with ~12px height:
```cpp
gainLabel_.setBounds(badgeX, badgeY + kGainBadgeH, kGainBadgeW, 12);
```

`kGainBadgeH` is already 20, so `gainLabel_` sits at `badgeY + 20` with the same width as the badge.

## Dependencies
None
