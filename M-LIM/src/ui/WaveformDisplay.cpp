#include "WaveformDisplay.h"
#include "Colours.h"
#include <cmath>

// dB grid lines drawn on background — shared constant from Colours.h
// WaveformDisplay uses the first 10 entries (0 to -27 dBFS); see MLIMColours::kMeterGridDB
static constexpr int kWaveformGridDBCount = 10;

// ─────────────────────────────────────────────────────────────────────────────
WaveformDisplay::WaveformDisplay()
    : history_ (kHistorySize)
{
    startTimerHz (60);
}

WaveformDisplay::~WaveformDisplay()
{
    stopTimer();
}

// ─────────────────────────────────────────────────────────────────────────────
void WaveformDisplay::pushMeterData (const MeterData& data)
{
    Frame f;
    // Use the louder channel for display; levels are linear 0-1
    f.inputLevel   = std::max (data.inputLevelL,  data.inputLevelR);
    f.outputLevel  = std::max (data.outputLevelL, data.outputLevelR);
    // gainReduction is stored as a non-positive dB value (0 = no GR, -6 = 6 dB GR)
    f.gainReduction = (data.gainReduction <= 0.0f) ? -data.gainReduction : data.gainReduction;

    history_[static_cast<std::size_t>(writePos_)] = f;
    writePos_ = (writePos_ + 1) % kHistorySize;
    if (frameCount_ < kHistorySize)
        ++frameCount_;
}

void WaveformDisplay::setDisplayMode (DisplayMode mode)
{
    if (displayMode_ == mode)
        return;
    displayMode_ = mode;
    if (onDisplayModeChanged)
        onDisplayModeChanged (mode);
    repaint();
}

// ─────────────────────────────────────────────────────────────────────────────
// Mode selector helpers
// ─────────────────────────────────────────────────────────────────────────────

juce::String WaveformDisplay::modeToString (DisplayMode mode)
{
    switch (mode)
    {
        case DisplayMode::Fast:     return "FAST";
        case DisplayMode::Slow:     return "SLOW";
        case DisplayMode::SlowDown: return "SLOWDOWN";
        case DisplayMode::Infinite: return "INFINITE";
        case DisplayMode::Off:      return "OFF";
        default:                    return "FAST";
    }
}

juce::Rectangle<float> WaveformDisplay::modeSelectorBounds() const noexcept
{
    // Top-left corner of the waveform content area (after the scale strip)
    return juce::Rectangle<float> (kScaleWidth + 4.0f, 4.0f, 70.0f, 14.0f);
}

void WaveformDisplay::mouseDown (const juce::MouseEvent& e)
{
    if (! modeSelectorBounds().contains (e.position))
        return;

    if (e.mods.isRightButtonDown())
    {
        juce::PopupMenu menu;
        menu.addItem (1, "Fast",     true, displayMode_ == DisplayMode::Fast);
        menu.addItem (2, "Slow",     true, displayMode_ == DisplayMode::Slow);
        menu.addItem (3, "SlowDown", true, displayMode_ == DisplayMode::SlowDown);
        menu.addItem (4, "Infinite", true, displayMode_ == DisplayMode::Infinite);
        menu.addItem (5, "Off",      true, displayMode_ == DisplayMode::Off);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
            [this] (int result)
            {
                switch (result)
                {
                    case 1: setDisplayMode (DisplayMode::Fast);     break;
                    case 2: setDisplayMode (DisplayMode::Slow);     break;
                    case 3: setDisplayMode (DisplayMode::SlowDown); break;
                    case 4: setDisplayMode (DisplayMode::Infinite); break;
                    case 5: setDisplayMode (DisplayMode::Off);      break;
                    default: break;
                }
            });
    }
    else
    {
        // Left-click: cycle through modes
        auto next = static_cast<int> (displayMode_) + 1;
        if (next > static_cast<int> (DisplayMode::Off))
            next = 0;
        setDisplayMode (static_cast<DisplayMode> (next));
    }
}

void WaveformDisplay::mouseEnter (const juce::MouseEvent& e)
{
    if (modeSelectorBounds().contains (e.position))
    {
        modeSelectorHovered_ = true;
        repaint();
    }
}

void WaveformDisplay::mouseExit (const juce::MouseEvent& /*e*/)
{
    if (modeSelectorHovered_)
    {
        modeSelectorHovered_ = false;
        repaint();
    }
}

void WaveformDisplay::mouseMove (const juce::MouseEvent& e)
{
    bool over = modeSelectorBounds().contains (e.position);
    if (over != modeSelectorHovered_)
    {
        modeSelectorHovered_ = over;
        repaint();
    }
}

void WaveformDisplay::setCeiling (float dB)
{
    ceilingDB_ = juce::jlimit (-30.0f, 0.0f, dB);
}

void WaveformDisplay::setPeakReadouts (float leftDB, float rightDB)
{
    peakReadoutL_ = leftDB;
    peakReadoutR_ = rightDB;
    // No explicit repaint needed — timer repaints at 60fps
}

// ─────────────────────────────────────────────────────────────────────────────
void WaveformDisplay::timerCallback()
{
    repaint();
}

void WaveformDisplay::resized() {}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

float WaveformDisplay::levelToY (float linear, const juce::Rectangle<float>& area) const noexcept
{
    // Convert linear amplitude to dBFS, then map to Y using the same scale as the grid:
    // 0 dBFS → top of area, -kMaxGRdB → bottom (consistent with drawBackground grid lines)
    float dBFS = (linear > 1e-6f)
        ? 20.0f * std::log10 (juce::jlimit (1e-6f, 1.0f, linear))
        : -kMaxGRdB;
    dBFS = juce::jlimit (-kMaxGRdB, 0.0f, dBFS);
    float frac = (-dBFS) / kMaxGRdB;   // 0 = top (0 dBFS), 1 = bottom (-kMaxGRdB)
    return area.getY() + frac * area.getHeight();
}

float WaveformDisplay::grToHeight (float grDB, const juce::Rectangle<float>& area) const noexcept
{
    float frac = juce::jlimit (0.0f, 1.0f, grDB / kMaxGRdB);
    return frac * area.getHeight();
}

void WaveformDisplay::forEachFrame (
    std::function<void(int col, const Frame&, int totalCols)> cb) const
{
    if (frameCount_ == 0) return;

    int total = std::min (frameCount_, kHistorySize);
    // oldest frame to display is (writePos_ - total + kHistorySize) % kHistorySize
    for (int i = 0; i < total; ++i)
    {
        int idx = (writePos_ - total + i + kHistorySize) % kHistorySize;
        cb (i, history_[static_cast<std::size_t> (idx)], total);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Paint
// ─────────────────────────────────────────────────────────────────────────────

void WaveformDisplay::paint (juce::Graphics& g)
{
    if (displayMode_ == DisplayMode::Off)
    {
        g.fillAll (MLIMColours::displayBackground);
        drawModeSelector (g);
        return;
    }

    // Reserve left strip for dB scale
    auto bounds = getLocalBounds().toFloat();
    auto scaleArea = bounds.removeFromLeft (kScaleWidth);
    auto displayArea = bounds;

    drawBackground     (g, displayArea);
    drawCeilingLine    (g, displayArea, scaleArea);
    drawOutputFill     (g, displayArea);
    drawInputFill      (g, displayArea);
    drawGainReduction  (g, displayArea);
    drawOutputEnvelope (g, displayArea);
    drawPeakMarkers    (g, displayArea);
    drawScale          (g, scaleArea);
    drawPeakReadouts   (g, displayArea);
    drawModeSelector   (g);
}

// ─────────────────────────────────────────────────────────────────────────────

void WaveformDisplay::drawModeSelector (juce::Graphics& g) const
{
    auto rect = modeSelectorBounds();
    juce::String label = modeToString (displayMode_);

    juce::Colour textCol = modeSelectorHovered_
        ? MLIMColours::textPrimary.withAlpha (0.9f)
        : MLIMColours::textSecondary.withAlpha (0.0f);

    // Subtle background pill on hover
    if (modeSelectorHovered_)
    {
        g.setColour (MLIMColours::waveformHoverOverlay);
        g.fillRoundedRectangle (rect, 3.0f);
    }

    g.setFont (juce::Font (MLIMColours::kFontSizeSmall));
    g.setColour (textCol);
    g.drawText (label, rect, juce::Justification::centredLeft, false);
}

void WaveformDisplay::drawPeakReadouts (juce::Graphics& g,
                                         const juce::Rectangle<float>& area) const
{
    // Two small boxes in top-right corner: [L: xxx dB] [R: xxx dB]
    static constexpr float kBoxW = 60.0f;
    static constexpr float kBoxH = 14.0f;
    static constexpr float kGap  =  3.0f;

    auto fmtPeak = [](float db) -> juce::String
    {
        if (db <= -96.0f) return "--- dB";
        return juce::String (db, 1) + " dB";
    };

    juce::String lStr = fmtPeak (peakReadoutL_);
    juce::String rStr = fmtPeak (peakReadoutR_);

    auto drawBox = [&] (float x, float y, const juce::String& text, bool isClipping)
    {
        auto box = juce::Rectangle<float> (x, y, kBoxW, kBoxH);
        g.setColour (MLIMColours::peakLabelBackground.withAlpha (0.85f));
        g.fillRect (box);
        g.setColour (MLIMColours::panelBorder);
        g.drawRect (box, 1.0f);
        g.setFont (juce::Font (MLIMColours::kFontSizeSmall));
        g.setColour (isClipping ? MLIMColours::meterDanger : MLIMColours::peakLabel);
        g.drawText (text, box.reduced (2.0f, 1.0f), juce::Justification::centredRight, false);
    };

    const float rightEdge = area.getRight() - kGap;
    const float topY      = area.getY() + kGap;
    drawBox (rightEdge - kBoxW,            topY, rStr, peakReadoutR_ >= -0.5f);
    drawBox (rightEdge - kBoxW * 2 - kGap, topY, lStr, peakReadoutL_ >= -0.5f);
}

void WaveformDisplay::drawBackground (juce::Graphics& g,
                                       const juce::Rectangle<float>& area) const
{
    // Subtle vertical gradient: near-black with blue tint at top, dark blue-gray at bottom
    juce::ColourGradient gradient = juce::ColourGradient::vertical (
        MLIMColours::displayGradientTop,
        area.getY(),
        MLIMColours::displayGradientBottom,
        area.getBottom());
    g.setGradientFill (gradient);
    g.fillRect (area);

    // Simulated idle fill — approximates the composite appearance of input waveform at ~-6 dBFS.
    // Pixel analysis shows reference is 30-50 counts brighter in the lower 56% of the display.
    // Fill starts at 44% from top (fillFrac=0.56), tapering from alpha 0.0 at top to 0.42 at bottom.
    {
        const float fillFrac = 0.56f;  // covers lower 56% of display height
        const float fillTop  = area.getBottom() - area.getHeight() * fillFrac;

        juce::ColourGradient fillGrad (
            MLIMColours::inputWaveform.withAlpha (0.0f),   0.0f, fillTop,
            MLIMColours::inputWaveform.withAlpha (0.82f),  0.0f, area.getBottom(),  // task-406: reduced from 0.88 to reduce blue cast
            false);
        g.setGradientFill (fillGrad);
        g.fillRect (area.getX(), fillTop, area.getWidth(), area.getBottom() - fillTop);
    }

    // Upper idle fill — approximates dense input waveform content in -6 to -12 dBFS zone.
    // Covers y=20%–50% height (centered around 30%), peak alpha 0.20.
    // Narrowed and darkened (task-450) to match Pro-L 2 near-black upper zone.
    {
        const float uTop = area.getY() + area.getHeight() * 0.20f;
        const float uMid = area.getY() + area.getHeight() * 0.30f;
        const float uBot = area.getY() + area.getHeight() * 0.50f;
        juce::Colour uFill = MLIMColours::inputWaveform.withAlpha (1.0f);

        juce::ColourGradient rGrad (
            uFill.withAlpha (0.0f),   0.0f, uTop,
            uFill.withAlpha (0.20f),  0.0f, uMid,  // task-450: reduced from 0.32 to darken upper zone
            false);
        g.setGradientFill (rGrad);
        g.fillRect (area.getX(), uTop, area.getWidth(), uMid - uTop);

        juce::ColourGradient fGrad (
            uFill.withAlpha (0.20f),  0.0f, uMid,  // task-450: reduced from 0.32
            uFill.withAlpha (0.0f),   0.0f, uBot,
            false);
        g.setGradientFill (fGrad);
        g.fillRect (area.getX(), uMid, area.getWidth(), uBot - uMid);
    }

    // Mid-zone brightness boost — two-pass tent centred at 58% height
    // Extends from 36%–82%, peak alpha 0.80 at midpoint to close 40-53 unit gap.
    {
        const float midTop   = area.getY() + area.getHeight() * 0.36f;
        const float midMid   = area.getY() + area.getHeight() * 0.58f;
        const float midBot   = area.getY() + area.getHeight() * 0.82f;
        juce::Colour midFill = MLIMColours::waveformIdleMidFill;

        // Rising half: transparent at 36% → 0.52 at 58% (task-422: reduced from 0.68)
        juce::ColourGradient riseGrad (
            midFill.withAlpha (0.0f),   0.0f, midTop,
            midFill.withAlpha (0.52f),  0.0f, midMid,
            false);
        g.setGradientFill (riseGrad);
        g.fillRect (area.getX(), midTop, area.getWidth(), midMid - midTop);

        // Falling half: 0.52 at 58% → transparent at 82% (task-422: reduced from 0.68)
        juce::ColourGradient fallGrad (
            midFill.withAlpha (0.52f),  0.0f, midMid,
            midFill.withAlpha (0.0f),   0.0f, midBot,
            false);
        g.setGradientFill (fallGrad);
        g.fillRect (area.getX(), midMid, area.getWidth(), midBot - midMid);
    }

    // Center tent brightness boost — 40%→58% rising, 58%→76% falling
    // Addresses the 39-unit mid-zone gap (ref #6D7694 vs current #474F6C).
    {
        const float cTop = area.getY() + area.getHeight() * 0.40f;
        const float cMid = area.getY() + area.getHeight() * 0.58f;
        const float cBot = area.getY() + area.getHeight() * 0.76f;
        juce::Colour cCol = MLIMColours::waveformIdleMidFill;  // same steel-blue as midFill above

        // Rising half: 0.0 at cTop → 0.40 at cMid (task-422: reduced from 0.52)
        juce::ColourGradient upGrad (
            cCol.withAlpha (0.0f),   0.0f, cTop,
            cCol.withAlpha (0.40f),  0.0f, cMid,
            false);
        g.setGradientFill (upGrad);
        g.fillRect (area.getX(), cTop, area.getWidth(), cMid - cTop);

        // Falling half: 0.40 at cMid → 0.0 at cBot (task-422: reduced from 0.52)
        juce::ColourGradient downGrad (
            cCol.withAlpha (0.40f),  0.0f, cMid,
            cCol.withAlpha (0.0f),   0.0f, cBot,
            false);
        g.setGradientFill (downGrad);
        g.fillRect (area.getX(), cMid, area.getWidth(), cBot - cMid);
    }

    // Lower idle fill — approximates output waveform/envelope content in lower portion
    // Covers y=62%–100%, peak alpha at bottom 0.25, fades to 0 at 62%
    {
        const float lTop = area.getY() + area.getHeight() * 0.62f;
        juce::Colour lFill = MLIMColours::waveformIdleLowFill;  // warmer neutral (equal R/G, less blue overshoot)

        juce::ColourGradient lGrad (
            lFill.withAlpha (0.0f),   0.0f, lTop,
            lFill.withAlpha (0.35f),  0.0f, area.getBottom(),
            false);
        g.setGradientFill (lGrad);
        g.fillRect (area.getX(), lTop, area.getWidth(), area.getBottom() - lTop);
    }

    // Horizontal dB grid lines
    g.setColour (MLIMColours::waveformGridLine.withAlpha (0.25f));
    for (int gi = 0; gi < kWaveformGridDBCount; ++gi)
    {
        const float db = MLIMColours::kMeterGridDB[gi];
        // Map dB to GR fraction (0 dB GR = top, kMaxGRdB GR = bottom)
        float frac = (-db) / kMaxGRdB;   // dB is negative for GR scale
        float y = area.getY() + frac * area.getHeight();
        if (y >= area.getY() && y <= area.getBottom())
            g.drawHorizontalLine (juce::roundToInt (y), area.getX(), area.getRight());
    }

    // dB overlay labels — left-aligned on the waveform left edge (Pro-L 2 style)
    g.setFont (juce::Font (MLIMColours::kFontSizeSmall - 1.0f));
    for (int gi = 0; gi < kWaveformGridDBCount; ++gi)
    {
        const float db  = MLIMColours::kMeterGridDB[gi];
        float frac      = (-db) / kMaxGRdB;
        float y         = area.getY() + frac * area.getHeight();
        if (y < area.getY() || y > area.getBottom()) continue;

        juce::String label = (db == 0.0f) ? "0 dB"
                                          : juce::String (juce::roundToInt (db)) + " dB";
        const float labelW = 38.0f;
        auto labelRect = juce::Rectangle<float> (
            area.getX() + 2.0f,
            y - 6.0f,
            labelW,
            12.0f);
        g.setColour (MLIMColours::textSecondary.withAlpha (0.55f));
        g.drawText (label, labelRect, juce::Justification::centredRight, false);
    }

    // Left-edge idle gradient removed (task-423): the pink-lavender tint (now
    // MLIMColours::waveformLeftEdgeTint) didn't match reference. Removal eliminates pink cast.
}

void WaveformDisplay::drawCeilingLine (juce::Graphics& g,
                                        const juce::Rectangle<float>& area,
                                        const juce::Rectangle<float>& scaleArea) const
{
    // Map ceiling dBFS to Y: 0 dBFS = top, -kMaxGRdB = bottom (same coordinate system as grid)
    float frac = (-ceilingDB_) / kMaxGRdB;
    float y    = area.getY() + frac * area.getHeight();
    y = juce::jlimit (area.getY(), area.getBottom(), y);

    // Solid ceiling line
    const juce::Colour lineColour = MLIMColours::waveformCeilingLine;
    const float lineX0   = area.getX();
    const float lineX1   = area.getRight();

    g.setColour (lineColour);
    g.drawLine (lineX0, y, lineX1, y, 1.5f);

    // Small ceiling label on the left scale area
    juce::String label = juce::String (ceilingDB_, 1) + " dB";
    auto labelRect = juce::Rectangle<float> (scaleArea.getX() + 2.0f,
                                              y - 6.0f,
                                              scaleArea.getWidth() - 4.0f,
                                              12.0f);
    g.setFont (juce::Font (MLIMColours::kFontSizeSmall));
    g.setColour (lineColour);
    g.drawText (label, labelRect, juce::Justification::centredRight, false);
}

void WaveformDisplay::drawFilledWaveformPath (juce::Graphics& g,
                                               juce::Colour colour,
                                               const juce::Rectangle<float>& area,
                                               std::function<float(const Frame&)> getY,
                                               bool closeAtTop) const
{
    if (frameCount_ == 0) return;

    juce::Path path;
    bool first = true;
    const float anchorY = closeAtTop ? area.getY() : area.getBottom();

    forEachFrame ([&] (int col, const Frame& f, int totalCols)
    {
        float x = area.getX() + col * (area.getWidth() / static_cast<float> (totalCols));
        float y = getY (f);
        if (first) { path.startNewSubPath (x, anchorY); first = false; }
        path.lineTo (x, y);
    });

    path.lineTo (area.getRight(), anchorY);
    path.closeSubPath();

    g.setColour (colour);
    g.fillPath (path);
}

void WaveformDisplay::drawSymmetricWaveformPath (juce::Graphics& g,
                                                  juce::Colour colour,
                                                  const juce::Rectangle<float>& area,
                                                  std::function<float(const Frame&)> getLevelY) const
{
    if (frameCount_ == 0) return;

    const float midY = area.getCentreY();

    // Top half: trace from midY upward
    juce::Path topPath;
    bool firstTop = true;
    forEachFrame ([&] (int col, const Frame& f, int totalCols)
    {
        float x = area.getX() + col * (area.getWidth() / static_cast<float> (totalCols));
        float h = std::max (0.0f, midY - getLevelY (f));
        if (firstTop) { topPath.startNewSubPath (x, midY); firstTop = false; }
        topPath.lineTo (x, midY - h);
    });
    topPath.lineTo (area.getRight(), midY);
    topPath.closeSubPath();

    // Bottom half: mirror of top
    juce::Path botPath;
    bool firstBot = true;
    forEachFrame ([&] (int col, const Frame& f, int totalCols)
    {
        float x = area.getX() + col * (area.getWidth() / static_cast<float> (totalCols));
        float h = std::max (0.0f, midY - getLevelY (f));
        if (firstBot) { botPath.startNewSubPath (x, midY); firstBot = false; }
        botPath.lineTo (x, midY + h);
    });
    botPath.lineTo (area.getRight(), midY);
    botPath.closeSubPath();

    g.setColour (colour);
    g.fillPath (topPath);
    g.fillPath (botPath);
}

void WaveformDisplay::drawOutputFill (juce::Graphics& g,
                                       const juce::Rectangle<float>& area) const
{
    drawSymmetricWaveformPath (g, MLIMColours::outputWaveform, area,
        [&] (const Frame& f) { return levelToY (f.outputLevel, area); });
}

void WaveformDisplay::drawInputFill (juce::Graphics& g,
                                      const juce::Rectangle<float>& area) const
{
    drawSymmetricWaveformPath (g, MLIMColours::inputWaveform, area,
        [&] (const Frame& f) { return levelToY (f.inputLevel, area); });
}

void WaveformDisplay::drawGainReduction (juce::Graphics& g,
                                          const juce::Rectangle<float>& area) const
{
    drawFilledWaveformPath (g, MLIMColours::gainReduction.withAlpha (0.82f), area,
        [&] (const Frame& f) { return area.getY() + grToHeight (f.gainReduction, area); },
        true /* closeAtTop */);
}

void WaveformDisplay::drawOutputEnvelope (juce::Graphics& g,
                                           const juce::Rectangle<float>& area) const
{
    if (frameCount_ == 0) return;

    const float midY = area.getCentreY();
    juce::Path path;
    bool first = true;

    forEachFrame ([&] (int col, const Frame& f, int totalCols)
    {
        float x = area.getX() + col * (area.getWidth() / static_cast<float> (totalCols));
        float h = std::max (0.0f, midY - levelToY (f.outputLevel, area));
        float y = midY - h;   // top edge of symmetric output fill
        if (first) { path.startNewSubPath (x, y); first = false; }
        else        path.lineTo (x, y);
    });

    g.setColour (MLIMColours::outputEnvelope);
    g.strokePath (path, juce::PathStrokeType (1.5f));
}

void WaveformDisplay::drawPeakMarkers (juce::Graphics& g,
                                        const juce::Rectangle<float>& area) const
{
    if (frameCount_ < 3) return;

    int total = std::min (frameCount_, kHistorySize);
    float colW = area.getWidth() / static_cast<float> (total);

    // Build flat array of input levels (reuse pre-allocated scratch buffer)
    auto& inp = mGrScratch_;
    forEachFrame ([&] (int col, const Frame& f, int /*totalCols*/)
    {
        inp[static_cast<std::size_t> (col)] = f.inputLevel;
    });

    g.setFont (juce::Font (MLIMColours::kFontSizeSmall));

    float lastLabelX = -100.0f; // track last drawn label x to avoid overlap

    for (int i = 1; i < total - 1; ++i)
    {
        float v = inp[static_cast<std::size_t> (i)];

        // Local maximum in input level above ~-6 dBFS threshold (linear 0.5)
        if (v > 0.5f
            && v >= inp[static_cast<std::size_t> (i - 1)]
            && v >= inp[static_cast<std::size_t> (i + 1)])
        {
            float x = area.getX() + i * colW;

            // Skip if too close to previous label
            if (x - lastLabelX < 15.0f)
                continue;

            float y    = levelToY (v, area);
            float dBFS = 20.0f * std::log10 (juce::jlimit (1e-6f, 1.0f, v));
            juce::String label = juce::String (dBFS, 1);

            float labelW = g.getCurrentFont().getStringWidthFloat (label) + 8.0f;
            // Position badge slightly above the peak
            auto bgRect = juce::Rectangle<float> (x - labelW * 0.5f, y - 16.0f, labelW, 14.0f);
            g.setColour (MLIMColours::peakLabel.withAlpha (0.85f));
            g.fillRoundedRectangle (bgRect, 3.0f);
            g.setColour (MLIMColours::peakLabelBackground);
            g.drawText (label, bgRect, juce::Justification::centred, false);

            lastLabelX = x;
        }
    }
}

void WaveformDisplay::drawScale (juce::Graphics& g,
                                  const juce::Rectangle<float>& area) const
{
    g.setColour (MLIMColours::background);
    g.fillRect (area);

    // Border on the right edge of the scale strip, separating it from the waveform
    g.setColour (MLIMColours::panelBorder);
    g.drawVerticalLine (juce::roundToInt (area.getRight()), area.getY(), area.getBottom());

    g.setFont (juce::Font (MLIMColours::kFontSizeSmall));

    // Reuse the same dB grid values as background
    for (int gi = 0; gi < kWaveformGridDBCount; ++gi)
    {
        const float db = MLIMColours::kMeterGridDB[gi];
        float frac = (-db) / kMaxGRdB;
        float y = area.getY() + frac * area.getHeight();
        if (y < area.getY() || y > area.getBottom()) continue;

        juce::String label = (db == 0.0f) ? "0" : juce::String (juce::roundToInt (db));
        auto labelRect = juce::Rectangle<float> (area.getX() + 2.0f,
                                                  y - 6.0f,
                                                  area.getWidth() - 4.0f,
                                                  12.0f);
        g.setColour (MLIMColours::textSecondary);
        g.drawText (label, labelRect, juce::Justification::centredRight, false);
    }
}
