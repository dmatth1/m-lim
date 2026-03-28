#pragma once

#include <juce_graphics/juce_graphics.h>

namespace MLIMColours
{
    // Background colours
    const juce::Colour background               { 0xff1A1A1A };
    const juce::Colour displayBackground        { 0xff111118 };
    const juce::Colour panelBorder              { 0xff333333 };
    const juce::Colour widgetBackground         { 0xff222230 };  // combo boxes, tooltips, etc.
    const juce::Colour peakLabelBackground      { 0xff1A1A1A };  // dark background behind peak labels
    const juce::Colour topBarPresetBackground   { 0xff2D2D2D };  // noticeably lighter than bar bg (0xff252525)
    const juce::Colour accentDarkBackground     { 0xff1E1E2A };  // dark navy tint for accent buttons
    const juce::Colour barTrackBackground       { 0xff2A2838 };  // dark blue-grey (task-426: brighten for reference match)

    // Waveform display gradient colours
    // These approximate the composite appearance of Pro-L 2 waveform with active audio.
    // Reference background pure color is lighter (~#8992AB) but the composite with
    // input/output waveform fills appears as a darker neutral-warm gray at top
    // and blue-saturated gray at bottom. Task-317 values gave 20.00% waveform RMSE.
    const juce::Colour displayGradientTop   { 0xff282020 };  // task-421: darken -8 units (R=40,G=32,B=32) to reduce upper zone brightness
    const juce::Colour displayGradientBottom{ 0xff9E9EC4 };  // +46R +20G +16B warmer for lower zone RMSE (task-408)

    // Waveform colours (with alpha)
    const juce::Colour inputWaveform        { 0xCC6878A0 };  // lighter steel-blue, ~80% alpha (composites to ~#5A6A8A matching reference)
    const juce::Colour outputWaveform       { 0x804060A0 };  // slightly deeper blue, ~50% alpha
    const juce::Colour outputEnvelope       { 0xCCE8C878 };  // warm amber/cream, ~80% alpha
    const juce::Colour gainReduction        { 0xffEE3333 };  // clear red (Pro-L 2 parity — GR fill is red in reference)
    const juce::Colour grMeterLow           { 0xffE8C840 };  // warm yellow (0–3 dB GR)
    const juce::Colour grMeterMid           { 0xffFF8C00 };  // warm orange (3–9 dB GR)
    const juce::Colour waveformGridLine     { 0xff9AA0B4 };  // horizontal dB grid lines — lighter than gradient, matches Pro-L 2 subtle grid
    const juce::Colour waveformIdleMidFill  { 0xff828AA5 };  // mid/center tent idle fill (steel blue)
    const juce::Colour waveformIdleLowFill  { 0xff9898A8 };  // lower idle fill (warm neutral)
    const juce::Colour waveformHoverOverlay { 0x30FFFFFF };  // mode selector hover highlight
    const juce::Colour waveformCeilingLine  { 0x50DD4444 };  // ceiling line — warm red, ~31% alpha — subtler, closer to Pro-L 2 reference

    // Label colours
    const juce::Colour peakLabel        { 0xffFFD700 };  // gold/yellow

    // Knob colours
    const juce::Colour knobFace         { 0xff585858 };  // neutral medium gray (matches Pro-L 2 silver)
    const juce::Colour knobFaceHighlight{ 0xffDDDDE8 };  // brighter blue-tinted highlight, ≈ ref #DBDBE4 (task-381)
    const juce::Colour knobFaceShadow   { 0xff484860 };  // bluer shadow — blue-gray tint across face (task-453)
    const juce::Colour knobArc          { 0xff70A0D0 };  // brighter blue arc, 3.0px stroke (task-454)
    const juce::Colour knobArcDim       { 0xff404040 };  // slightly darker dim track for better contrast with face
    const juce::Colour knobPointer      { 0xffFFFFFF };
    const juce::Colour sliderFill       { 0xff2D7EE8 };  // accent blue for linear slider fills

    // Text colours
    const juce::Colour textPrimary      { 0xffE0E0E0 };
    const juce::Colour textSecondary    { 0xff9E9E9E };

    // Meter colours
    const juce::Colour meterSafe        { 0xff6879A0 };  // muted steel-blue (matches Pro-L 2 safe zone — was 0xff4D88CC vivid blue)
    const juce::Colour meterWarning     { 0xffFFD54F };  // yellow
    const juce::Colour meterDanger      { 0xffFF5252 };  // red
    const juce::Colour meterAtTarget    { 0xffFF8C00 };  // orange — at target ±2 LU
    const juce::Colour histogramHighlight{ 0xff2A2A3A }; // target level row highlight in histogram
    const juce::Colour lufsReadoutGood  { 0xffE87828 };  // warm orange for LUFS readout — matches Pro-L 2 reference orange (was golden-yellow)

    // Loudness panel background (dark purple-gray, matches Pro-L 2 right panel ~#2B2729)
    const juce::Colour loudnessPanelBackground { 0xff363244 };  // task-429: darkened −16 to match reference ~(58,53,63)

    // Loudness panel histogram area gradient (decoupled from waveform gradient so each can be tuned independently)
    const juce::Colour loudnessHistogramTop    { 0xff383848 };  // task-429: darkened −16 to match reference
    const juce::Colour loudnessHistogramBottom { 0xff303040 };  // task-429: darkened −16 to match reference

    // Control strip gradient background (dark purple-gray, matching Pro-L 2's knob area)
    const juce::Colour controlStripTop    { 0xff5D5D6A };  // task-427: +8 brightness (R=93, G=93, B=106)
    const juce::Colour controlStripBottom { 0xff444350 };  // task-427: +8 brightness (R=68, G=67, B=80)

    // Top bar gradient background
    const juce::Colour topBarGradientTop    { 0xff4A4650 };
    const juce::Colour topBarGradientBottom { 0xff3C3842 };

    // Button colours
    const juce::Colour algoButtonInactive       { 0xff545870 };  // +24R +8G -15B lighten for better match (task-409)
    const juce::Colour algoButtonSelected       { 0xff2E3E58 };  // selected algo button — dark navy, subtle blue highlight (task-346)
    const juce::Colour buttonBackground         { 0xff242424 };  // default button/toggle background
    const juce::Colour buttonOnBackground       { 0xff1A4A1A };  // toggle-on background (dark green)
    const juce::Colour buttonOnText             { 0xff66DD66 };  // toggle-on text (bright green)
    const juce::Colour buttonPressedBackground  { 0xff3A3A3A };  // active/pressed state background

    // Panel overlay
    const juce::Colour panelOverlay      { 0x20FFFFFF };  // semi-transparent overlay for panels

    // Accent
    const juce::Colour accentBlue       { 0xff2D7EE8 };

    // Font sizes
    static constexpr float kFontSizeSmall  = 9.0f;
    static constexpr float kFontSizeMedium = 10.0f;
    static constexpr float kFontSizeLarge  = 11.0f;

    // dBFS grid markings used across meter and waveform displays (3 dB steps, 0 to -30)
    static constexpr float kMeterGridDB[] = {
        0.0f, -3.0f, -6.0f, -9.0f, -12.0f,
        -15.0f, -18.0f, -21.0f, -24.0f, -27.0f, -30.0f
    };
    static constexpr int kMeterGridDBCount = 11;

    // Shared helper: draws LED-style segment separator lines across a bar area.
    // Call after setting no colour — this sets its own colour via sepColour.
    inline void drawSegmentSeparators (juce::Graphics& g,
                                       float x, float barTop, float barH, float barW,
                                       float segH, float segGap, juce::Colour sepColour)
    {
        g.setColour (sepColour);
        for (float sy = barTop; sy < barTop + barH; sy += segH + segGap)
            g.fillRect (x, sy + segH, barW, segGap);
    }
}
