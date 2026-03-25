#include "WaveformDisplay.h"
#include "Colours.h"
#include <cmath>

// dB grid lines drawn on background
static const float kGridDB[] = { 0.0f, -3.0f, -6.0f, -9.0f, -12.0f,
                                 -15.0f, -18.0f, -21.0f, -24.0f };

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
    displayMode_ = mode;
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
    // Top-left corner of the waveform area, small label region
    return juce::Rectangle<float> (4.0f, 4.0f, 70.0f, 14.0f);
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

    // Reserve right strip for dB scale
    auto bounds = getLocalBounds().toFloat();
    auto scaleArea = bounds.removeFromRight (kScaleWidth);
    auto displayArea = bounds;

    drawBackground     (g, displayArea);
    drawCeilingLine    (g, displayArea, scaleArea);
    drawOutputFill     (g, displayArea);
    drawInputFill      (g, displayArea);
    drawGainReduction  (g, displayArea);
    drawOutputEnvelope (g, displayArea);
    drawPeakMarkers    (g, displayArea);
    drawScale          (g, scaleArea);
    drawModeSelector   (g);
}

// ─────────────────────────────────────────────────────────────────────────────

void WaveformDisplay::drawModeSelector (juce::Graphics& g) const
{
    auto rect = modeSelectorBounds();
    juce::String label = modeToString (displayMode_);

    juce::Colour textCol = modeSelectorHovered_
        ? MLIMColours::textPrimary.withAlpha (0.9f)
        : MLIMColours::textSecondary.withAlpha (0.7f);

    // Subtle background pill on hover
    if (modeSelectorHovered_)
    {
        g.setColour (juce::Colour (0x30FFFFFF));
        g.fillRoundedRectangle (rect, 3.0f);
    }

    g.setFont (juce::Font (9.0f));
    g.setColour (textCol);
    g.drawText (label, rect, juce::Justification::centredLeft, false);
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

    // Horizontal dB grid lines
    g.setColour (juce::Colour (0xff2A2A2A));
    for (float db : kGridDB)
    {
        // Map dB to GR fraction (0 dB GR = top, kMaxGRdB GR = bottom)
        float frac = (-db) / kMaxGRdB;   // dB is negative for GR scale
        float y = area.getY() + frac * area.getHeight();
        if (y >= area.getY() && y <= area.getBottom())
            g.drawHorizontalLine (juce::roundToInt (y), area.getX(), area.getRight());
    }
}

void WaveformDisplay::drawCeilingLine (juce::Graphics& g,
                                        const juce::Rectangle<float>& area,
                                        const juce::Rectangle<float>& scaleArea) const
{
    // Map ceiling dBFS to Y: 0 dBFS = top, -kMaxGRdB = bottom (same coordinate system as grid)
    float frac = (-ceilingDB_) / kMaxGRdB;
    float y    = area.getY() + frac * area.getHeight();
    y = juce::jlimit (area.getY(), area.getBottom(), y);

    // Dashed ceiling line: subtle white with moderate alpha
    const juce::Colour lineColour { 0xAAFFFFFF };
    const float dashLen  = 6.0f;
    const float gapLen   = 4.0f;
    const float lineX0   = area.getX();
    const float lineX1   = area.getRight();

    g.setColour (lineColour);
    float x = lineX0;
    bool drawing = true;
    while (x < lineX1)
    {
        float segEnd = std::min (x + (drawing ? dashLen : gapLen), lineX1);
        if (drawing)
            g.drawLine (x, y, segEnd, y, 1.0f);
        x = segEnd;
        drawing = !drawing;
    }

    // Small ceiling label on the right edge of scale area
    juce::String label = juce::String (ceilingDB_, 1) + "dB";
    auto labelRect = juce::Rectangle<float> (scaleArea.getX() + 2.0f,
                                              y - 6.0f,
                                              scaleArea.getWidth() - 4.0f,
                                              12.0f);
    g.setFont (juce::Font (9.0f));
    g.setColour (lineColour);
    g.drawText (label, labelRect, juce::Justification::centredLeft, false);
}

void WaveformDisplay::drawOutputFill (juce::Graphics& g,
                                       const juce::Rectangle<float>& area) const
{
    if (frameCount_ == 0) return;

    int total = std::min (frameCount_, kHistorySize);
    float colW = area.getWidth() / static_cast<float> (total);

    juce::Path path;
    bool first = true;

    forEachFrame ([&] (int col, const Frame& f, int totalCols)
    {
        float x = area.getX() + col * (area.getWidth() / static_cast<float> (totalCols));
        float y = levelToY (f.outputLevel, area);
        if (first) { path.startNewSubPath (x, area.getBottom()); first = false; }
        path.lineTo (x, y);
        (void) colW;
    });

    path.lineTo (area.getRight(), area.getBottom());
    path.closeSubPath();

    g.setColour (MLIMColours::outputWaveform);
    g.fillPath (path);
}

void WaveformDisplay::drawInputFill (juce::Graphics& g,
                                      const juce::Rectangle<float>& area) const
{
    if (frameCount_ == 0) return;

    int total = std::min (frameCount_, kHistorySize);

    juce::Path path;
    bool first = true;

    forEachFrame ([&] (int col, const Frame& f, int totalCols)
    {
        float x = area.getX() + col * (area.getWidth() / static_cast<float> (totalCols));
        float y = levelToY (f.inputLevel, area);
        if (first) { path.startNewSubPath (x, area.getBottom()); first = false; }
        path.lineTo (x, y);
        (void) total;
    });

    path.lineTo (area.getRight(), area.getBottom());
    path.closeSubPath();

    g.setColour (MLIMColours::inputWaveform);
    g.fillPath (path);
}

void WaveformDisplay::drawGainReduction (juce::Graphics& g,
                                          const juce::Rectangle<float>& area) const
{
    if (frameCount_ == 0) return;

    int total = std::min (frameCount_, kHistorySize);

    juce::Path path;
    bool first = true;

    forEachFrame ([&] (int col, const Frame& f, int totalCols)
    {
        float x = area.getX() + col * (area.getWidth() / static_cast<float> (totalCols));
        float h = grToHeight (f.gainReduction, area);
        float y = area.getY() + h;

        if (first)
        {
            path.startNewSubPath (x, area.getY());
            first = false;
        }
        path.lineTo (x, y);
        (void) total;
    });

    // Close along the top edge
    path.lineTo (area.getRight(), area.getY());
    path.closeSubPath();

    g.setColour (MLIMColours::gainReduction.withAlpha (0.75f));
    g.fillPath (path);
}

void WaveformDisplay::drawOutputEnvelope (juce::Graphics& g,
                                           const juce::Rectangle<float>& area) const
{
    if (frameCount_ == 0) return;

    int total = std::min (frameCount_, kHistorySize);

    juce::Path path;
    bool first = true;

    forEachFrame ([&] (int col, const Frame& f, int totalCols)
    {
        float x = area.getX() + col * (area.getWidth() / static_cast<float> (totalCols));
        float y = levelToY (f.outputLevel, area);
        if (first) { path.startNewSubPath (x, y); first = false; }
        else        path.lineTo (x, y);
        (void) total;
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

    // Build flat arrays for peak detection (reuse pre-allocated scratch buffer)
    auto& gr = mGrScratch_;
    forEachFrame ([&] (int col, const Frame& f, int /*totalCols*/)
    {
        gr[static_cast<std::size_t> (col)] = f.gainReduction;
    });

    g.setColour (MLIMColours::peakLabel);
    g.setFont (juce::Font (10.0f));

    for (int i = 1; i < total - 1; ++i)
    {
        // Local maximum with a minimum threshold
        if (gr[static_cast<std::size_t> (i)] > 0.5f
            && gr[static_cast<std::size_t> (i)] >= gr[static_cast<std::size_t> (i - 1)]
            && gr[static_cast<std::size_t> (i)] >= gr[static_cast<std::size_t> (i + 1)])
        {
            float x    = area.getX() + i * colW;
            float h    = grToHeight (gr[static_cast<std::size_t> (i)], area);
            float y    = area.getY() + h;
            float grDB = gr[static_cast<std::size_t> (i)];
            juce::String label = juce::String (grDB, 1) + "dB";

            float labelW = g.getCurrentFont().getStringWidthFloat (label) + 8.0f;
            auto bgRect = juce::Rectangle<float> (x - labelW * 0.5f, y + 2.0f, labelW, 14.0f);
            g.setColour (MLIMColours::peakLabel.withAlpha (0.85f));
            g.fillRoundedRectangle (bgRect, 3.0f);
            g.setColour (juce::Colour (0xff1A1A1A));
            g.setFont (juce::Font (9.0f));
            g.drawText (label, bgRect, juce::Justification::centred, false);
        }
    }
}

void WaveformDisplay::drawScale (juce::Graphics& g,
                                  const juce::Rectangle<float>& area) const
{
    g.setColour (juce::Colour (0xff1E1E1E));
    g.fillRect (area);

    g.setColour (MLIMColours::panelBorder);
    g.drawVerticalLine (juce::roundToInt (area.getX()), area.getY(), area.getBottom());

    g.setFont (juce::Font (9.0f));

    // Reuse the same dB grid values as background
    for (float db : kGridDB)
    {
        float frac = (-db) / kMaxGRdB;
        float y = area.getY() + frac * area.getHeight();
        if (y < area.getY() || y > area.getBottom()) continue;

        juce::String label = (db == 0.0f) ? "0" : juce::String (juce::roundToInt (db));
        auto labelRect = juce::Rectangle<float> (area.getX() + 2.0f,
                                                  y - 6.0f,
                                                  area.getWidth() - 4.0f,
                                                  12.0f);
        g.setColour (MLIMColours::textSecondary);
        g.drawText (label, labelRect, juce::Justification::centredLeft, false);
    }
}
