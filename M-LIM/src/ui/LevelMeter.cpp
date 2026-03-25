#include "LevelMeter.h"
#include "Colours.h"

namespace
{
    // Scale markings shown on the meter (dBFS) — uniform 3 dB steps
    constexpr float kScaleMarks[] = { 0.0f, -3.0f, -6.0f, -9.0f, -12.0f,
                                      -15.0f, -18.0f, -21.0f, -24.0f, -27.0f, -30.0f };

    // Width of each channel bar relative to available width
    constexpr float kBarWidthRatio = 0.42f;
    // Gap between the two bars
    constexpr float kGapRatio = 0.08f;
    // Width reserved for scale labels on the right
    constexpr int   kScaleWidth = 22;
    // Peak hold line thickness
    constexpr float kPeakLineH  = 2.0f;
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

//==============================================================================
float LevelMeter::dbToNorm (float db) noexcept
{
    return juce::jlimit (0.0f, 1.0f, (db - kMinDB) / (kMaxDB - kMinDB));
}

//==============================================================================
void LevelMeter::drawChannel (juce::Graphics& g,
                               juce::Rectangle<float> bar,
                               float levelDB,
                               float peakDB) const
{
    const float barH   = bar.getHeight();
    const float barTop = bar.getY();

    // Background
    g.setColour (MLIMColours::displayBackground);
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

    // danger zone
    if (fillTop < dangerBot)
    {
        float top  = juce::jmax (fillTop, dangerTop);
        float bot  = dangerBot;
        g.setColour (MLIMColours::meterDanger);
        g.fillRect (bar.withTop (top).withBottom (bot));
    }

    // warning zone
    if (fillTop < warnBot && dangerBot < barTop + fillH + barTop)
    {
        float top = juce::jmax (fillTop, warnTop);
        float bot = juce::jmin (warnBot, barTop + barH);
        if (top < bot)
        {
            g.setColour (MLIMColours::meterWarning);
            g.fillRect (bar.withTop (top).withBottom (bot));
        }
    }

    // safe zone
    if (fillTop < safeBot)
    {
        float top = juce::jmax (fillTop, warnBot);
        float bot = safeBot;
        if (top < bot)
        {
            g.setColour (MLIMColours::meterSafe);
            g.fillRect (bar.withTop (top).withBottom (bot));
        }
    }

    // --- peak hold line ---
    if (peakDB > kMinDB + 0.5f)
    {
        const float normPeak  = dbToNorm (peakDB);
        const float peakY     = barTop + barH * (1.0f - normPeak) - kPeakLineH * 0.5f;
        g.setColour (juce::Colours::white);
        g.fillRect (bar.getX(), peakY, bar.getWidth(), kPeakLineH);
    }

    // Thin border
    g.setColour (MLIMColours::panelBorder);
    g.drawRect (bar, 1.0f);
}

void LevelMeter::drawScale (juce::Graphics& g) const
{
    const auto bounds = getLocalBounds().toFloat();
    const float barH  = bounds.getHeight();
    const float barY  = bounds.getY();

    g.setFont (juce::Font (9.0f));
    g.setColour (MLIMColours::textSecondary);

    const float scaleX = bounds.getRight() - (float) kScaleWidth;

    for (float mark : kScaleMarks)
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

void LevelMeter::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float availW = bounds.getWidth() - (float) kScaleWidth;
    const float h      = bounds.getHeight();
    const float x      = bounds.getX();
    const float y      = bounds.getY();

    const float barW = availW * kBarWidthRatio;
    const float gap  = availW * kGapRatio;

    auto barL = juce::Rectangle<float> (x, y, barW, h);
    auto barR = juce::Rectangle<float> (x + barW + gap, y, barW, h);

    drawChannel (g, barL, levelL_, peakL_);
    drawChannel (g, barR, levelR_, peakR_);
    drawScale (g);
}
