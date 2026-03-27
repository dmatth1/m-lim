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

void GainReductionMeter::mouseDown (const juce::MouseEvent&)
{
    resetPeakGR();
}

void GainReductionMeter::mouseMove (const juce::MouseEvent&)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
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
    auto barArea = getLocalBounds().toFloat();

    // Background — dark fill matching reference Pro-L 2 (~#241B20 ≈ barTrackBackground)
    g.setColour (MLIMColours::barTrackBackground);
    g.fillRect (barArea);

    drawBar      (g, barArea);
    drawPeakTick (g, barArea);
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
    const float barTop = barArea.getY();
    const float barH   = barArea.getHeight();
    MLIMColours::drawSegmentSeparators (g, barArea.getX(), barTop, barH, barArea.getWidth(),
                                        kSegH, kSegGap,
                                        MLIMColours::barTrackBackground.brighter (0.35f));

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

