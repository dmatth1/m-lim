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
    const juce::Colour topBarPresetBackground   { 0xff232323 };  // slightly lighter background for preset label
    const juce::Colour accentDarkBackground     { 0xff1E1E2A };  // dark navy tint for accent buttons
    const juce::Colour barTrackBackground       { 0xff222222 };  // background track behind progress bars

    // Waveform display gradient colours
    // Reference samples from Pro-L 2: top ~#8992AB, middle ~#6F7790 (measured from v1-0005.png)
    const juce::Colour displayGradientTop   { 0xff5A6075 };  // medium blue-gray (matches Pro-L 2 idle waveform BG)
    const juce::Colour displayGradientBottom{ 0xff3E4258 };  // slightly darker blue-gray at bottom

    // Waveform colours (with alpha)
    const juce::Colour inputWaveform        { 0xCC6878A0 };  // lighter steel-blue, ~80% alpha (composites to ~#5A6A8A matching reference)
    const juce::Colour outputWaveform       { 0x804060A0 };  // slightly deeper blue, ~50% alpha
    const juce::Colour outputEnvelope       { 0xCCE8C878 };  // warm amber/cream, ~80% alpha
    const juce::Colour gainReduction        { 0xffEE3333 };  // clear red (Pro-L 2 parity — GR fill is red in reference)
    const juce::Colour waveformGridLine     { 0xff474D62 };  // horizontal dB grid lines — darker than background for subtle visibility
    const juce::Colour waveformHoverOverlay { 0x30FFFFFF };  // mode selector hover highlight
    const juce::Colour waveformCeilingLine  { 0xCCDD4444 };  // ceiling line — warm red, ~80% alpha (matches Pro-L 2 red ceiling)

    // Label colours
    const juce::Colour peakLabel        { 0xffFFD700 };  // gold/yellow

    // Knob colours
    const juce::Colour knobFace         { 0xff4A526A };  // steel blue-grey (was 3A3A3A)
    const juce::Colour knobFaceHighlight{ 0xff7080A0 };  // knob highlight for 3D gradient
    const juce::Colour knobFaceShadow   { 0xff303448 };  // knob shadow for 3D gradient
    const juce::Colour knobArc          { 0xff4888C8 };  // medium blue arc for clear value visibility
    const juce::Colour knobArcDim       { 0xff555555 };  // dim grey for inactive/minimum arc tick marks
    const juce::Colour knobPointer      { 0xffFFFFFF };
    const juce::Colour sliderFill       { 0xff2D7EE8 };  // accent blue for linear slider fills

    // Text colours
    const juce::Colour textPrimary      { 0xffE0E0E0 };
    const juce::Colour textSecondary    { 0xff9E9E9E };

    // Meter colours
    const juce::Colour meterSafe        { 0xff4D88CC };  // medium cyan-blue (matches Pro-L 2 reference level bar colour)
    const juce::Colour meterWarning     { 0xffFFD54F };  // yellow
    const juce::Colour meterDanger      { 0xffFF5252 };  // red
    const juce::Colour meterAtTarget    { 0xffFF8C00 };  // orange — at target ±2 LU
    const juce::Colour histogramHighlight{ 0xff2A2A3A }; // target level row highlight in histogram
    const juce::Colour lufsReadoutGood  { 0xffE8C040 };  // warm golden-yellow for LUFS readout (below target)

    // Control strip gradient background (dark purple-gray, matching Pro-L 2's knob area)
    const juce::Colour controlStripTop    { 0xff3B3840 };  // dark purple-gray top — matches reference ~#3A3641
    const juce::Colour controlStripBottom { 0xff282530 };  // darker at bottom — matches reference ~#2A2734

    // Button colours
    const juce::Colour algoButtonInactive       { 0xff303848 };  // inactive algo button — dark blue-gray blending with control strip
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
