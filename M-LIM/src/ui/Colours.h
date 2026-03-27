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
    const juce::Colour barTrackBackground       { 0xff181818 };  // background track behind progress bars

    // Waveform display gradient colours
    // These approximate the composite appearance of Pro-L 2 waveform with active audio.
    // Reference background pure color is lighter (~#8992AB) but the composite with
    // input/output waveform fills appears as a darker neutral-warm gray at top
    // and blue-saturated gray at bottom. Task-317 values gave 20.00% waveform RMSE.
    const juce::Colour displayGradientTop   { 0xff686468 };  // neutral/warm gray, best composite match
    const juce::Colour displayGradientBottom{ 0xff506090 };  // blue-saturated, matches ref composite bottom (task-341 revert: #4A4F6B caused regression 22.05%→22.99%)

    // Waveform colours (with alpha)
    const juce::Colour inputWaveform        { 0xCC6878A0 };  // lighter steel-blue, ~80% alpha (composites to ~#5A6A8A matching reference)
    const juce::Colour outputWaveform       { 0x804060A0 };  // slightly deeper blue, ~50% alpha
    const juce::Colour outputEnvelope       { 0xCCE8C878 };  // warm amber/cream, ~80% alpha
    const juce::Colour gainReduction        { 0xffEE3333 };  // clear red (Pro-L 2 parity — GR fill is red in reference)
    const juce::Colour grMeterLow           { 0xffE8C840 };  // warm yellow (0–3 dB GR)
    const juce::Colour grMeterMid           { 0xffFF8C00 };  // warm orange (3–9 dB GR)
    const juce::Colour waveformGridLine     { 0xff9AA0B4 };  // horizontal dB grid lines — lighter than gradient, matches Pro-L 2 subtle grid
    const juce::Colour waveformHoverOverlay { 0x30FFFFFF };  // mode selector hover highlight
    const juce::Colour waveformCeilingLine  { 0x50DD4444 };  // ceiling line — warm red, ~31% alpha — subtler, closer to Pro-L 2 reference

    // Label colours
    const juce::Colour peakLabel        { 0xffFFD700 };  // gold/yellow

    // Knob colours
    const juce::Colour knobFace         { 0xff585858 };  // neutral medium gray (matches Pro-L 2 silver)
    const juce::Colour knobFaceHighlight{ 0xffB8B8CC };  // brighter blue-tinted highlight (matches reference metallic sheen ~#DBDBE7)
    const juce::Colour knobFaceShadow   { 0xff303030 };  // neutral dark shadow
    const juce::Colour knobArc          { 0xff6898C8 };  // slightly lighter blue, more subtle at 2.0px
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
    const juce::Colour lufsReadoutGood  { 0xffE8C040 };  // warm golden-yellow for LUFS readout (below target)

    // Loudness panel background (dark purple-gray, matches Pro-L 2 right panel ~#2B2729)
    const juce::Colour loudnessPanelBackground { 0xff2B2729 };  // dark purple-gray; reverted from 0xff1E1C21 (task-331) which worsened right panel RMSE

    // Control strip gradient background (dark purple-gray, matching Pro-L 2's knob area)
    const juce::Colour controlStripTop    { 0xff4A4756 };  // brightened top — closer to Pro-L 2 ref ~36% brightness
    const juce::Colour controlStripBottom { 0xff38353F };  // brightened bottom — ≈ 22% brightness

    // Button colours
    const juce::Colour algoButtonInactive       { 0xff303848 };  // inactive algo button — dark blue-gray blending with control strip
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
}
