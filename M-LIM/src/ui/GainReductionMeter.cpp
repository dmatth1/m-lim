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

    drawBar      (g, barArea);
    drawPeakTick (g, barArea);
    drawScale    (g, scaleArea);
    drawNumeric  (g, numArea);
}

void GainReductionMeter::drawBar (juce::Graphics& g,
                                   const juce::Rectangle<float>& barArea) const
{
    if (currentGR_ <= 0.0f) return;

    float fillH = grToFrac (currentGR_) * barArea.getHeight();
    auto filled = barArea.withHeight (fillH);   // top-anchored

    // Gradient: slightly brighter at top where GR is highest
    juce::ColourGradient grad (
        MLIMColours::gainReduction,
        filled.getTopLeft(),
        MLIMColours::gainReduction.darker (0.3f),
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
    g.setColour (juce::Colour (0xff1E1E1E));
    g.fillRect (scaleArea);

    g.setColour (MLIMColours::panelBorder);
    g.drawVerticalLine (juce::roundToInt (scaleArea.getX()),
                        scaleArea.getY(), scaleArea.getBottom());

    g.setFont (juce::Font (9.0f));

    // Marks at 0, -3, -6, -9, -12, -18, -24 dB
    static const float kMarks[] = { 0.0f, 3.0f, 6.0f, 9.0f, 12.0f, 18.0f, 24.0f };
    for (float mark : kMarks)
    {
        if (mark > maxGRdB_) break;
        float frac = grToFrac (mark);
        float y    = scaleArea.getY() + frac * scaleArea.getHeight();
        juce::String label = (mark == 0.0f) ? "0" : "-" + juce::String (juce::roundToInt (mark));
        auto labelRect = juce::Rectangle<float> (scaleArea.getX() + 2.0f,
                                                  y - 5.0f,
                                                  scaleArea.getWidth() - 4.0f,
                                                  10.0f);
        g.setColour (MLIMColours::textSecondary);
        g.drawText (label, labelRect, juce::Justification::centredLeft, false);
    }
}

void GainReductionMeter::drawNumeric (juce::Graphics& g,
                                       const juce::Rectangle<float>& numArea) const
{
    g.setColour (juce::Colour (0xff1A1A1A));
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

    g.setFont (juce::Font (10.0f, juce::Font::bold));
    g.setColour (MLIMColours::gainReduction);
    g.drawText (curStr, cur, juce::Justification::centred, false);

    g.setFont (juce::Font (9.0f));
    g.setColour (MLIMColours::peakLabel);
    g.drawText (pkStr, pk, juce::Justification::centred, false);
}
