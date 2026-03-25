#include "LoudnessPanel.h"
#include "Colours.h"
#include <cmath>
#include <limits>

// ─────────────────────────────────────────────────────────────────────────────

LoudnessPanel::LoudnessPanel()
{
    resetButton_.setClickingTogglesState (false);
    resetButton_.setColour (juce::TextButton::buttonColourId,  juce::Colour (0xff2A2A2A));
    resetButton_.setColour (juce::TextButton::textColourOffId, MLIMColours::textSecondary);
    resetButton_.onClick = [this] { if (onResetIntegrated) onResetIntegrated(); };
    addAndMakeVisible (resetButton_);
}

// ── Data setters ─────────────────────────────────────────────────────────────

void LoudnessPanel::setMomentary     (float lufs) { momentary_     = lufs; repaint(); }
void LoudnessPanel::setShortTerm     (float lufs) { shortTerm_     = lufs; repaint(); }
void LoudnessPanel::setIntegrated    (float lufs) { integrated_    = lufs; repaint(); }
void LoudnessPanel::setLoudnessRange (float lu)   { loudnessRange_ = lu;   repaint(); }
void LoudnessPanel::setTruePeak      (float dBTP) { truePeak_      = dBTP; repaint(); }
void LoudnessPanel::setTarget        (float lufs) { targetLUFS_    = lufs; repaint(); }

// ─────────────────────────────────────────────────────────────────────────────

void LoudnessPanel::resized()
{
    // Place reset button next to the Integrated row
    int rowY = kPadding + kRowH * 2;   // 0-indexed row 2 = Integrated
    auto btnBounds = juce::Rectangle<int> (
        getWidth() - kBtnW - kPadding,
        rowY + (kRowH - 18) / 2,
        kBtnW,
        18);
    resetButton_.setBounds (btnBounds);
}

// ─────────────────────────────────────────────────────────────────────────────

void LoudnessPanel::paint (juce::Graphics& g)
{
    // Background
    g.setColour (MLIMColours::displayBackground);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);

    g.setColour (MLIMColours::panelBorder);
    g.drawRoundedRectangle (getLocalBounds().reduced (1).toFloat(), 4.0f, 1.0f);

    float targetFrac = lufsToFrac (targetLUFS_);

    // Row 0: Momentary
    drawRow (g,
             getLocalBounds().withTrimmedTop (kPadding + kRowH * 0).withHeight (kRowH),
             "Momentary",
             fmtLUFS (momentary_),
             lufsToFrac (momentary_),
             true,
             targetFrac);

    // Row 1: Short-Term
    drawRow (g,
             getLocalBounds().withTrimmedTop (kPadding + kRowH * 1).withHeight (kRowH),
             "Short-Term",
             fmtLUFS (shortTerm_),
             lufsToFrac (shortTerm_),
             true,
             targetFrac);

    // Row 2: Integrated (reset button is a child component, drawn on top)
    drawRow (g,
             getLocalBounds().withTrimmedTop (kPadding + kRowH * 2).withHeight (kRowH),
             "Integrated",
             fmtLUFS (integrated_),
             lufsToFrac (integrated_),
             true,
             targetFrac);

    // Row 3: Range (no bar)
    {
        juce::String val = std::isinf (loudnessRange_)
                         ? "---"
                         : juce::String (loudnessRange_, 1);
        drawRow (g,
                 getLocalBounds().withTrimmedTop (kPadding + kRowH * 3).withHeight (kRowH),
                 "Range",
                 val + " LU",
                 -1.0f,   // no bar
                 false,
                 0.0f);
    }

    // Row 4: True Peak (no bar)
    drawRow (g,
             getLocalBounds().withTrimmedTop (kPadding + kRowH * 4).withHeight (kRowH),
             "True Peak",
             fmtDBTP (truePeak_),
             -1.0f,
             false,
             0.0f);
}

// ─────────────────────────────────────────────────────────────────────────────

void LoudnessPanel::drawRow (juce::Graphics& g,
                              const juce::Rectangle<int>& rowBounds,
                              const juce::String& label,
                              const juce::String& valueStr,
                              float  barFill,
                              bool   showTarget,
                              float  targetFrac) const
{
    auto r = rowBounds.reduced (kPadding, 2);

    // Label
    g.setColour (MLIMColours::textSecondary);
    g.setFont (juce::Font (10.0f));
    g.drawText (label, r.removeFromLeft (kLabelW), juce::Justification::centredLeft, false);

    // Value
    g.setColour (MLIMColours::textPrimary);
    g.setFont (juce::Font (11.0f, juce::Font::bold));
    g.drawText (valueStr, r.removeFromLeft (kValueW), juce::Justification::centredRight, false);

    // Bar (optional)
    if (barFill >= 0.0f)
    {
        r.removeFromLeft (4);  // small gap
        // Reserve space for reset button if this is the Integrated row
        auto barRect = r.toFloat();
        drawBar (g, barRect, barFill, showTarget, targetFrac);
    }
}

void LoudnessPanel::drawBar (juce::Graphics& g,
                              const juce::Rectangle<float>& barBounds,
                              float fill,
                              bool  showTarget,
                              float targetFrac) const
{
    // Background track
    g.setColour (juce::Colour (0xff222222));
    g.fillRoundedRectangle (barBounds, 2.0f);

    if (fill <= 0.0f) return;

    float fillW = juce::jlimit (0.0f, barBounds.getWidth(), fill * barBounds.getWidth());
    auto filled = barBounds.withWidth (fillW);

    // Color zones
    juce::Colour barColour;
    if (fill < 0.70f)
        barColour = MLIMColours::meterSafe;
    else if (fill < 0.90f)
        barColour = MLIMColours::meterWarning;
    else
        barColour = MLIMColours::meterDanger;

    g.setColour (barColour);
    g.fillRoundedRectangle (filled, 2.0f);

    // Target reference line
    if (showTarget && targetFrac > 0.0f && targetFrac < 1.0f)
    {
        float tx = barBounds.getX() + targetFrac * barBounds.getWidth();
        g.setColour (MLIMColours::peakLabel.withAlpha (0.8f));
        g.drawVerticalLine (juce::roundToInt (tx), barBounds.getY(), barBounds.getBottom());
    }
}

// ─────────────────────────────────────────────────────────────────────────────

float LoudnessPanel::lufsToFrac (float lufs) noexcept
{
    if (std::isinf (lufs) || std::isnan (lufs) || lufs < kMinLUFS)
        return 0.0f;
    return juce::jlimit (0.0f, 1.0f, (lufs - kMinLUFS) / (-kMinLUFS));
}

juce::String LoudnessPanel::fmtLUFS (float lufs)
{
    if (std::isinf (lufs) || std::isnan (lufs) || lufs < -200.0f)
        return "---";
    return juce::String (lufs, 1);
}

juce::String LoudnessPanel::fmtDBTP (float dBTP)
{
    if (std::isinf (dBTP) || std::isnan (dBTP) || dBTP < -200.0f)
        return "---";
    return juce::String (dBTP, 1) + " dBTP";
}
