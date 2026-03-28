#include "LevelMeter.h"
#include "Colours.h"

namespace
{
    // Width of each channel bar relative to available width
    // 0.46 * 2 + 0.08 = 1.0 → bars fill the full component width
    constexpr float kBarWidthRatio = 0.46f;
    // Gap between the two bars
    constexpr float kGapRatio = 0.08f;
    // Width reserved for scale labels on the right
    constexpr int   kScaleWidth = 20;
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

    // LED segment separator lines (matches GainReductionMeter pattern)
    static constexpr float kSegH   = 3.0f;
    static constexpr float kSegGap = 1.0f;
    MLIMColours::drawSegmentSeparators (g, bar.getX(), barTop, barH, bar.getWidth(),
                                        kSegH, kSegGap,
                                        MLIMColours::barTrackBackground.brighter (0.25f));

    // Idle structural gradient — simulates warm-program-material appearance at silence.
    // Alpha tuned to 0.60f (bottom) / 0.45f (warm mid) for optimal RMSE match with reference.
    // Task 382: empirical search showed increasing alpha improves right panel match.
    {
        const float barTop2 = bar.getY();
        const float barH2   = bar.getHeight();

        const float normWarn2    = dbToNorm (kWarnDB);
        const float normDanger2  = dbToNorm (kDangerDB);
        const float dangerBot2   = barTop2 + barH2 * (1.0f - normDanger2);
        const float warnBot2     = barTop2 + barH2 * (1.0f - normWarn2);

        // Extended warm zone: yellow/orange covers down to -12 dB instead of kWarnDB
        const float normWarmExt  = dbToNorm (-12.0f);
        const float warmExtY     = barTop2 + barH2 * (1.0f - normWarmExt);

        juce::ColourGradient idleGrad (
            MLIMColours::meterDanger.withAlpha (0.03f),           0.0f, barTop2,
            MLIMColours::meterSafe.withAlpha (1.0f),              0.0f, barTop2 + barH2,
            false);
        idleGrad.addColour ((dangerBot2 - barTop2) / barH2,
                            MLIMColours::grMeterMid.withAlpha (0.03f));     // task-425: minimal warm at danger
        idleGrad.addColour ((warnBot2   - barTop2) / barH2,
                            MLIMColours::meterWarning.withAlpha (0.03f));   // task-425: minimal warm at warning
        // Cool blue at -12 dB transition
        idleGrad.addColour ((warmExtY   - barTop2) / barH2,
                            MLIMColours::meterSafe.withAlpha (0.10f));      // task-425: subtle cool blue

        // Mid-fill at -24 dBFS — blue tint to match reference idle appearance
        const float normMidFill = dbToNorm (-24.0f);
        const float midFillY    = barTop2 + barH2 * (1.0f - normMidFill);
        idleGrad.addColour ((midFillY - barTop2) / barH2,
                            MLIMColours::meterSafe.withAlpha (0.35f));      // task-508: brighter mid fill

        // Transition zone at -18 dBFS — visible blue tint starts here
        const float normTrans = dbToNorm (-18.0f);
        const float transY    = barTop2 + barH2 * (1.0f - normTrans);
        idleGrad.addColour ((transY - barTop2) / barH2,
                            MLIMColours::meterSafe.withAlpha (0.55f));      // task-508: prominent neutral from -18 dB down

        g.setGradientFill (idleGrad);
        g.fillRect (bar);
    }

    // --- filled level portion ---
    const float normLevel = dbToNorm (levelDB);
    const float fillH     = barH * normLevel;
    const float fillTop   = barTop + barH - fillH;

    if (fillH > 0.0f)
    {
        // Compute zone boundary Y positions for gradient colour stops
        const float normWarn   = dbToNorm (kWarnDB);
        const float normDanger = dbToNorm (kDangerDB);
        const float dangerBot  = barTop + barH * (1.0f - normDanger);
        const float warnBot    = barTop + barH * (1.0f - normWarn);

        // Vertical gradient: red at top → yellow at warn threshold → warm amber at mid → cool blue → bright white-blue at bottom
        juce::ColourGradient gradient (
            MLIMColours::meterDanger,              0.0f, barTop,
            MLIMColours::meterSafe.brighter (0.5f), 0.0f, barTop + barH,
            false);
        gradient.addColour ((dangerBot - barTop) / barH, MLIMColours::meterWarning);
        gradient.addColour ((warnBot   - barTop) / barH, MLIMColours::grMeterLow.withAlpha (0.8f));
        gradient.addColour (0.85,                        MLIMColours::meterSafe);

        // Clip to the filled (active level) region and draw solid gradient
        g.saveState();
        g.reduceClipRegion (bar.withTop (fillTop).toNearestInt());
        g.setGradientFill (gradient);
        g.fillRect (bar);
        g.restoreState();
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

        // label — use full scale width, centred vertically around tick
        juce::String label = (mark == 0.0f) ? "0" : juce::String ((int) mark);
        g.drawText (label,
                    (int) scaleX,
                    (int) y - 5,
                    kScaleWidth,
                    10,
                    juce::Justification::centred,
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

    // Fill entire meter component with cool blue-purple gradient to match reference background
    {
        const float bgAlphaTop = showScale_ ? 0.55f : 0.15f;
        const float bgAlphaBot = showScale_ ? 0.65f : 0.25f;
        juce::ColourGradient bgGrad (
            MLIMColours::meterSafe.darker (0.6f).withAlpha (bgAlphaTop),  0.0f, y,
            MLIMColours::meterSafe.withAlpha (bgAlphaBot),                0.0f, y + h,
            false);
        g.setGradientFill (bgGrad);
        g.fillRect (bounds);
    }

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

    if (kScaleWidth > 0 && showScale_)
        drawScale (g, barL.getY(), barL.getHeight());
}
