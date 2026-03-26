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

    // Background — dark fill matching reference Pro-L 2 (~#241B20 ≈ barTrackBackground)
    g.setColour (MLIMColours::barTrackBackground);
    g.fillRect (barArea);

    drawBar (g, barArea);
    drawPeakTick   (g, barArea);
    drawScale      (g, scaleArea);
    drawNumeric    (g, numArea);
}

void GainReductionMeter::drawBar (juce::Graphics& g,
                                   const juce::Rectangle<float>& barArea) const
{
    // Constants matching LevelMeter segment style
    static constexpr float kSegH   = 3.0f;
    static constexpr float kSegGap = 1.0f;

    // 1. Background track — dark fill matching reference Pro-L 2 (~#241B20 ≈ barTrackBackground)
    g.setColour (MLIMColours::barTrackBackground);
    g.fillRect (barArea);

    // 2. Segment-separator texture across full bar height (LED strip look even at idle)
    g.setColour (MLIMColours::barTrackBackground.brighter (0.35f));
    const float barTop = barArea.getY();
    const float barH   = barArea.getHeight();
    for (float sy = barTop; sy < barTop + barH; sy += kSegH + kSegGap)
        g.fillRect (barArea.getX(), sy + kSegH, barArea.getWidth(), kSegGap);

    if (currentGR_ <= 0.0f) return;

    // 3. Filled level portion as discrete segments (top-anchored: 0 dB GR = top)
    const float fillH  = grToFrac (currentGR_) * barH;
    const float fillBot = barTop + fillH;

    // GR bar uses zone-based colours: yellow (0–3 dB), orange (3–9 dB), red (9+ dB)
    auto drawSegments = [&] (juce::Colour colour, float top, float bot)
    {
        if (top >= bot) return;
        g.setColour (colour);
        for (float sy = top; sy < bot; sy += kSegH + kSegGap)
        {
            float segBot = juce::jmin (sy + kSegH, bot);
            if (segBot > sy)
                g.fillRect (barArea.getX(), sy, barArea.getWidth(), segBot - sy);
        }
    };

    float zone1Bot = barTop + std::min (fillH, grToFrac (3.0f) * barH);  // 0–3 dB
    float zone2Bot = barTop + std::min (fillH, grToFrac (9.0f) * barH);  // 3–9 dB
    float zone3Bot = barTop + fillH;                                       // 9+ dB

    drawSegments (MLIMColours::grMeterLow,    barTop,    zone1Bot);  // warm yellow
    drawSegments (MLIMColours::grMeterMid,    zone1Bot,  zone2Bot);  // warm orange
    drawSegments (MLIMColours::gainReduction, zone2Bot,  zone3Bot);  // red
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
    if (kScaleW <= 0) return;

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

    g.setFont (juce::Font (MLIMColours::kFontSizeSmall));
    g.setColour (MLIMColours::textPrimary);
    g.drawText (curStr, cur, juce::Justification::centred, false);

    g.setFont (juce::Font (8.0f));
    g.setColour (MLIMColours::peakLabel);
    g.drawText (pkStr, pk, juce::Justification::centred, false);
}
