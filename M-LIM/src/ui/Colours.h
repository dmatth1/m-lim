#pragma once

#include <juce_graphics/juce_graphics.h>

namespace MLIMColours
{
    // Background colours
    const juce::Colour background       { 0xff1E1E1E };
    const juce::Colour displayBackground{ 0xff141414 };
    const juce::Colour panelBorder      { 0xff333333 };

    // Waveform colours (with alpha)
    const juce::Colour inputWaveform    { 0x994FC3F7 };  // light cyan, alpha ~0.6
    const juce::Colour outputWaveform   { 0x801565C0 };  // dark blue, alpha ~0.5
    const juce::Colour gainReduction    { 0xffFF4444 };  // bright red

    // Label colours
    const juce::Colour peakLabel        { 0xffFFD700 };  // gold/yellow

    // Knob colours
    const juce::Colour knobFace         { 0xff3A3A3A };
    const juce::Colour knobArc          { 0xff4FC3F7 };
    const juce::Colour knobPointer      { 0xffFFFFFF };

    // Text colours
    const juce::Colour textPrimary      { 0xffE0E0E0 };
    const juce::Colour textSecondary    { 0xff9E9E9E };

    // Meter colours
    const juce::Colour meterSafe        { 0xff4FC3F7 };  // blue
    const juce::Colour meterWarning     { 0xffFFD54F };  // yellow
    const juce::Colour meterDanger      { 0xffFF5252 };  // red

    // Accent
    const juce::Colour accentBlue       { 0xff2196F3 };
}
