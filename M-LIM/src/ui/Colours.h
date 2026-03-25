#pragma once

#include <juce_graphics/juce_graphics.h>

namespace MLIMColours
{
    // Background colours
    const juce::Colour background           { 0xff1E1E1E };
    const juce::Colour displayBackground    { 0xff141414 };
    const juce::Colour panelBorder          { 0xff333333 };

    // Waveform display gradient colours
    const juce::Colour displayGradientTop   { 0xff0D0D12 };  // near-black with slight blue tint
    const juce::Colour displayGradientBottom{ 0xff1A1A2E };  // dark blue-gray

    // Waveform colours (with alpha)
    const juce::Colour inputWaveform    { 0x70202840 };  // very dark navy, alpha ~44% (was bright cyan)
    const juce::Colour outputWaveform   { 0x60182848 };  // slightly darker navy, alpha ~38%
    const juce::Colour outputEnvelope   { 0x80B89040 };  // semi-transparent amber/tan envelope line
    const juce::Colour gainReduction    { 0xffFF4444 };  // bright red

    // Label colours
    const juce::Colour peakLabel        { 0xffFFD700 };  // gold/yellow

    // Knob colours
    const juce::Colour knobFace         { 0xff505872 };  // steel blue-grey (was 3A3A3A)
    const juce::Colour knobFaceHighlight{ 0xff7080A0 };  // knob highlight for 3D gradient
    const juce::Colour knobFaceShadow   { 0xff303448 };  // knob shadow for 3D gradient
    const juce::Colour knobArc          { 0xff5C6A84 };  // subtle blue-grey arc
    const juce::Colour knobPointer      { 0xffFFFFFF };
    const juce::Colour sliderFill       { 0xff2196F3 };  // accent blue for linear slider fills

    // Text colours
    const juce::Colour textPrimary      { 0xffE0E0E0 };
    const juce::Colour textSecondary    { 0xff9E9E9E };

    // Meter colours
    const juce::Colour meterSafe        { 0xff8896AC };  // pale steel grey-blue
    const juce::Colour meterWarning     { 0xffFFD54F };  // yellow
    const juce::Colour meterDanger      { 0xffFF5252 };  // red

    // Control strip gradient background (blue-gray, matching Pro-L 2's knob area)
    const juce::Colour controlStripTop    { 0xff3A3D4A };  // medium blue-gray (top)
    const juce::Colour controlStripBottom { 0xff2A2D3A };  // darker blue-gray (bottom)

    // Accent
    const juce::Colour accentBlue       { 0xff2196F3 };
}
