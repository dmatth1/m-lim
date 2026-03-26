#include "LevelMeter.h"
#include "Colours.h"

namespace
{
    // Width of each channel bar relative to available width
    constexpr float kBarWidthRatio = 0.42f;
    // Gap between the two bars
    constexpr float kGapRatio = 0.08f;
    // Width reserved for scale labels on the right
    constexpr int   kScaleWidth = 18;
    // Peak hold line thickness
    constexpr float kPeakLineH  = 2.0f;
    // Segmented LED-strip appearance
    constexpr float kSegH   = 3.0f;  // segment height in pixels
    constexpr float kSegGap = 1.0f;  // gap between segments
}

//==============================================================================
LevelMeter::LevelMeter()
{
    setOpaque (false);
}

//==============================================================================
void LevelMeter::setLevel (float leftDB, float rightDB)
{
    levelL_ = juce::jlimit (kMinDB, kMaxDB, leftDB);
    levelR_ = juce::jlimit (kMinDB, kMaxDB, rightDB);
    repaint();
}

void LevelMeter::setPeakHold (float leftDB, float rightDB)
{
    peakL_ = juce::jlimit (kMinDB, kMaxDB, leftDB);
    peakR_ = juce::jlimit (kMinDB, kMaxDB, rightDB);
    repaint();
}

void LevelMeter::resetPeakHold()
{
    peakL_ = kMinDB;
    peakR_ = kMinDB;
    repaint();
}

void LevelMeter::setClip (bool left, bool right)
{
    if (left)  clipL_ = true;
    if (right) clipR_ = true;
    repaint();
}

void LevelMeter::resetClip()
{
    clipL_ = false;
    clipR_ = false;
    repaint();
}

//==============================================================================
float LevelMeter::dbToNorm (float db) noexcept
{
    return juce::jlimit (0.0f, 1.0f, (db - kMinDB) / (kMaxDB - kMinDB));
}

//==============================================================================
void LevelMeter::drawChannel (juce::Graphics& g,
                               juce::Rectangle<float> bar,
                               float levelDB,
                               float peakDB,
                               bool  clipped) const
{
    const float barH   = bar.getHeight();
    const float barTop = bar.getY();

    // Background track
    g.setColour (MLIMColours::barTrackBackground);
    g.fillRect (bar);

    // --- filled level portion ---
    const float normLevel = dbToNorm (levelDB);
    const float fillH     = barH * normLevel;
    const float fillTop   = barTop + barH - fillH;

    // Split into danger / warning / safe zones
    const float normWarn   = dbToNorm (kWarnDB);
    const float normDanger = dbToNorm (kDangerDB);

    const float dangerTop  = barTop;
    const float dangerBot  = barTop + barH * (1.0f - normDanger);
    const float warnTop    = dangerBot;
    const float warnBot    = barTop + barH * (1.0f - normWarn);
    const float safeBot    = barTop + barH;  // bottom of safe zone = bottom of bar

    // Helper lambda: draw segmented fill between [top, bot) with given colour
    auto drawSegments = [&] (juce::Colour colour, float top, float bot)
    {
        if (top >= bot) return;
        g.setColour (colour);
        for (float sy = top; sy < bot; sy += kSegH + kSegGap)
        {
            float segBottom = juce::jmin (sy + kSegH, bot);
            if (segBottom > sy)
                g.fillRect (bar.withTop (sy).withBottom (segBottom));
        }
    };

    // danger zone
    if (fillTop < dangerBot)
    {
        float top = juce::jmax (fillTop, dangerTop);
        drawSegments (MLIMColours::meterDanger, top, dangerBot);
    }

    // warning zone
    if (fillTop < warnBot)
    {
        float top = juce::jmax (fillTop, warnTop);
        float bot = juce::jmin (warnBot, barTop + barH);
        drawSegments (MLIMColours::meterWarning, top, bot);
    }

    // safe zone
    if (fillTop < safeBot)
    {
        float top = juce::jmax (fillTop, warnBot);
        drawSegments (MLIMColours::meterSafe, top, safeBot);
    }

    // --- peak hold line ---
    if (peakDB > kMinDB + 0.5f)
    {
        const float normPeak  = dbToNorm (peakDB);
        const float peakY     = barTop + barH * (1.0f - normPeak) - kPeakLineH * 0.5f;
        g.setColour (juce::Colours::white);
        g.fillRect (bar.getX(), peakY, bar.getWidth(), kPeakLineH);
    }

    // --- clip indicator at the very top of the bar ---
    constexpr float clipBoxH = 4.0f;
    if (clipped)
        g.setColour (MLIMColours::meterDanger);
    else
        g.setColour (MLIMColours::displayBackground);
    g.fillRect (bar.getX(), barTop, bar.getWidth(), clipBoxH);

    // Thin border
    g.setColour (MLIMColours::panelBorder);
    g.drawRect (bar, 1.0f);
}

void LevelMeter::drawPeakLabel (juce::Graphics& g,
                                const juce::Rectangle<float>& labelArea,
                                float peakDB) const
{
    juce::String text = (peakDB <= kMinDB + 0.5f)
                      ? "---"
                      : juce::String (peakDB, 1);

    juce::Colour col = (peakDB >= kDangerDB) ? MLIMColours::meterDanger
                     : (peakDB >= kWarnDB)   ? MLIMColours::meterWarning
                                             : MLIMColours::textSecondary;
    g.setFont (juce::Font (MLIMColours::kFontSizeSmall));
    g.setColour (col);
    g.drawText (text, labelArea, juce::Justification::centred, false);
}

void LevelMeter::drawScale (juce::Graphics& g, float barTop, float barH) const
{
    const auto bounds = getLocalBounds().toFloat();
    const float barY  = barTop;

    g.setFont (juce::Font (MLIMColours::kFontSizeSmall));
    g.setColour (MLIMColours::textSecondary);

    const float scaleX = bounds.getRight() - (float) kScaleWidth;

    for (float mark : MLIMColours::kMeterGridDB)
    {
        const float norm = dbToNorm (mark);
        const float y    = barY + barH * (1.0f - norm);

        // tick
        g.drawLine (scaleX, y, scaleX + 4.0f, y, 1.0f);

        // label
        juce::String label = (mark == 0.0f) ? "0" : juce::String ((int) mark);
        g.drawText (label,
                    (int) scaleX + 5,
                    (int) y - 5,
                    kScaleWidth - 5,
                    10,
                    juce::Justification::left,
                    false);
    }
}

//==============================================================================
void LevelMeter::resized() {}

void LevelMeter::mouseDown (const juce::MouseEvent& e)
{
    // Reset clip indicators if user clicks anywhere on the meter
    // (Pro-L style: clicking the meter clears clip latches)
    resetClip();
    juce::ignoreUnused (e);
}

void LevelMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float availW = bounds.getWidth() - (float) kScaleWidth;
    const float h      = bounds.getHeight();
    const float x      = bounds.getX();
    const float y      = bounds.getY();

    const float barW = availW * kBarWidthRatio;
    const float gap  = availW * kGapRatio;

    // Reserve peak label strip at the top of each bar
    auto barL = juce::Rectangle<float> (x, y, barW, h);
    auto barR = juce::Rectangle<float> (x + barW + gap, y, barW, h);

    auto peakLabelL = barL.removeFromTop (kPeakLabelH);
    auto peakLabelR = barR.removeFromTop (kPeakLabelH);

    drawPeakLabel (g, peakLabelL, peakL_);
    drawPeakLabel (g, peakLabelR, peakR_);
    drawChannel (g, barL, levelL_, peakL_, clipL_);
    drawChannel (g, barR, levelR_, peakR_, clipR_);
    drawScale (g, barL.getY(), barL.getHeight());
}
