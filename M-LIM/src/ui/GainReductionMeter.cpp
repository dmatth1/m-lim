#include "GainReductionMeter.h"
#include "Colours.h"

GainReductionMeter::GainReductionMeter()
{
    setOpaque (false);
}

void GainReductionMeter::setGainReduction (float dB)
{
    currentGR_ = juce::jlimit (0.0f, maxGRdB_, dB);
    repaint();
}

void GainReductionMeter::setPeakGR (float dB)
{
    peakGR_ = juce::jlimit (0.0f, maxGRdB_, dB);
    repaint();
}

void GainReductionMeter::resetPeakGR()
{
    peakGR_ = 0.0f;
    repaint();
}

void GainReductionMeter::setRange (float maxGRdB)
{
    maxGRdB_ = juce::jmax (1.0f, maxGRdB);
    repaint();
}

void GainReductionMeter::setInputLevel (float leftDB, float rightDB)
{
    inputLevelL_ = leftDB;
    inputLevelR_ = rightDB;
    repaint();
}

void GainReductionMeter::resized() {}

juce::Rectangle<float> GainReductionMeter::peakLabelArea() const
{
    auto bounds  = getLocalBounds().toFloat();
    auto numArea = bounds.removeFromTop (static_cast<float> (kNumericH));
    // The peak readout occupies the lower half of numArea (matches drawNumeric)
    return numArea.withHeight (numArea.getHeight() * 0.5f)
                  .translated (0.0f, numArea.getHeight() * 0.5f);
}

void GainReductionMeter::mouseDown (const juce::MouseEvent& e)
{
    if (peakLabelArea().contains (e.position))
        resetPeakGR();
}

void GainReductionMeter::mouseMove (const juce::MouseEvent& e)
{
    if (peakLabelArea().contains (e.position))
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    else
        setMouseCursor (juce::MouseCursor::NormalCursor);
}

void GainReductionMeter::mouseExit (const juce::MouseEvent&)
{
    setMouseCursor (juce::MouseCursor::NormalCursor);
}

// ─────────────────────────────────────────────────────────────────────────────

float GainReductionMeter::grToFrac (float grDB) const noexcept
{
    return juce::jlimit (0.0f, 1.0f, grDB / maxGRdB_);
}

float GainReductionMeter::inputLevelToFrac (float dBFS) const noexcept
{
    // Maps kInputMinDB → 0.0 (empty) and kInputMaxDB → 1.0 (full height from bottom)
    return juce::jlimit (0.0f, 1.0f,
        (dBFS - kInputMinDB) / (kInputMaxDB - kInputMinDB));
}

// ─────────────────────────────────────────────────────────────────────────────

void GainReductionMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Numeric readout at top
    auto numArea   = bounds.removeFromTop (static_cast<float> (kNumericH));
    // Scale labels on the right
    auto scaleArea = bounds.removeFromRight  (static_cast<float> (kScaleW));
    auto barArea   = bounds;

    // Background
    g.setColour (MLIMColours::displayBackground);
    g.fillRect (barArea);

    drawInputLevel (g, barArea);
    drawBar        (g, barArea);
    drawPeakTick   (g, barArea);
    drawScale      (g, scaleArea);
    drawNumeric    (g, numArea);
}

void GainReductionMeter::drawInputLevel (juce::Graphics& g,
                                          const juce::Rectangle<float>& barArea) const
{
    // Draw L and R input level bars rising from the bottom, each occupying half the bar width
    const float halfW = barArea.getWidth() * 0.5f;

    auto drawChannel = [&] (float dBFS, float xOffset)
    {
        float frac = inputLevelToFrac (dBFS);
        if (frac <= 0.0f)
            return;

        float fillH = frac * barArea.getHeight();
        auto filled = juce::Rectangle<float> (
            barArea.getX() + xOffset,
            barArea.getBottom() - fillH,
            halfW,
            fillH);

        g.setColour (MLIMColours::meterSafe);
        g.fillRect (filled);
    };

    drawChannel (inputLevelL_, 0.0f);
    drawChannel (inputLevelR_, halfW);
}

void GainReductionMeter::drawBar (juce::Graphics& g,
                                   const juce::Rectangle<float>& barArea) const
{
    if (currentGR_ <= 0.0f) return;

    float fillH = grToFrac (currentGR_) * barArea.getHeight();
    auto filled = barArea.withHeight (fillH);   // top-anchored

    // Gradient: slightly brighter at top where GR is highest; ~0.8 alpha to overlay input level bars
    auto grColour    = MLIMColours::gainReduction.withAlpha (0.8f);
    auto grColourDim = MLIMColours::gainReduction.darker (0.3f).withAlpha (0.8f);
    juce::ColourGradient grad (
        grColour,
        filled.getTopLeft(),
        grColourDim,
        filled.getBottomLeft(),
        false);
    g.setGradientFill (grad);
    g.fillRect (filled);
}

void GainReductionMeter::drawPeakTick (juce::Graphics& g,
                                        const juce::Rectangle<float>& barArea) const
{
    if (peakGR_ <= 0.0f) return;

    float tickY = barArea.getY() + grToFrac (peakGR_) * barArea.getHeight();

    g.setColour (MLIMColours::peakLabel);
    g.drawHorizontalLine (juce::roundToInt (tickY),
                          barArea.getX(),
                          barArea.getRight());
}

void GainReductionMeter::drawScale (juce::Graphics& g,
                                     const juce::Rectangle<float>& scaleArea) const
{
    g.setColour (MLIMColours::background);
    g.fillRect (scaleArea);

    g.setColour (MLIMColours::panelBorder);
    g.drawVerticalLine (juce::roundToInt (scaleArea.getX()),
                        scaleArea.getY(), scaleArea.getBottom());

    g.setFont (juce::Font (8.0f));

    // Marks at 0, -3, -6, -9, -12, -18, -24 dB (labels shown without leading minus)
    static const float kMarks[] = { 0.0f, 3.0f, 6.0f, 9.0f, 12.0f, 18.0f, 24.0f };
    for (float mark : kMarks)
    {
        if (mark > maxGRdB_) break;
        float frac = grToFrac (mark);
        float y    = scaleArea.getY() + frac * scaleArea.getHeight();
        juce::String label = juce::String (juce::roundToInt (mark));
        auto labelRect = juce::Rectangle<float> (scaleArea.getX() + 1.0f,
                                                  y - 5.0f,
                                                  scaleArea.getWidth() - 2.0f,
                                                  10.0f);
        g.setColour (MLIMColours::textSecondary);
        g.drawText (label, labelRect, juce::Justification::centredLeft, false);
    }
}

void GainReductionMeter::drawNumeric (juce::Graphics& g,
                                       const juce::Rectangle<float>& numArea) const
{
    g.setColour (MLIMColours::peakLabelBackground);
    g.fillRect (numArea);

    // Current GR
    juce::String curStr = (currentGR_ > 0.0f)
                        ? "-" + juce::String (currentGR_, 1)
                        : "0.0";

    // Peak GR
    juce::String pkStr  = (peakGR_ > 0.0f)
                        ? "-" + juce::String (peakGR_, 1)
                        : "0.0";

    auto cur = numArea.withHeight (numArea.getHeight() * 0.5f);
    auto pk  = cur.withY (cur.getBottom());

    g.setFont (juce::Font (MLIMColours::kFontSizeLarge, juce::Font::bold));
    g.setColour (MLIMColours::textPrimary);
    g.drawText (curStr, cur, juce::Justification::centred, false);

    g.setFont (juce::Font (MLIMColours::kFontSizeSmall));
    g.setColour (MLIMColours::peakLabel);
    g.drawText (pkStr, pk, juce::Justification::centred, false);
}
