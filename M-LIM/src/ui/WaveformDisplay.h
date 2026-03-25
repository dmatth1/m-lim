#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "../dsp/MeterData.h"

/**
 * WaveformDisplay — real-time scrolling waveform with gain reduction overlay.
 *
 * Layers (back to front):
 *  1. Dark background + horizontal dB grid lines
 *  2. Output level filled area (dark navy, semi-transparent)
 *  3. Input level filled area (very dark navy, semi-transparent)
 *  4. Gain reduction bars from top (bright red)
 *  5. Output envelope line (amber/tan single-pixel line at output waveform edge)
 *  6. Peak markers: gold labels at GR peaks
 *  7. Vertical dB scale on right edge
 *
 * Call pushMeterData() from the UI timer (~60 fps) after draining the FIFO.
 * The component repaints itself via its own juce::Timer at 60 fps.
 */
class WaveformDisplay : public juce::Component,
                        public juce::Timer
{
public:
    /** Display mode matching the "displayMode" APVTS parameter. */
    enum class DisplayMode { Fast = 0, Slow, SlowDown, Infinite, Off };

    WaveformDisplay();
    ~WaveformDisplay() override;

    /** Append one meter snapshot (called on the UI/message thread). */
    void pushMeterData (const MeterData& data);

    /** Change the scrolling mode. */
    void setDisplayMode (DisplayMode mode);

    /** Update the output ceiling reference line (dBFS, e.g. -0.1). */
    void setCeiling (float dB);

    // Component / Timer overrides
    void paint  (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    // ── History ring buffer ───────────────────────────────────────────────
    static constexpr int kHistorySize = 1024;   // columns of history kept

    struct Frame
    {
        float inputLevel   = 0.0f;   // normalised 0-1 (peak of L+R)
        float outputLevel  = 0.0f;
        float gainReduction = 0.0f;  // positive dB e.g. 3.0 = 3 dB GR
    };

    std::vector<Frame> history_;   // ring buffer
    int writePos_ = 0;             // next write index
    int frameCount_ = 0;           // how many frames have been written (up to kHistorySize)

    DisplayMode displayMode_ = DisplayMode::Fast;
    float ceilingDB_ = -0.1f;   // output ceiling reference line (dBFS)

    // ── Helpers ───────────────────────────────────────────────────────────
    /** Map a linear 0-1 level to a Y coordinate (0 = bottom, 1 = top of display). */
    float levelToY (float linear, const juce::Rectangle<float>& area) const noexcept;

    /** Map a gain-reduction dB value to a bar height in pixels from the top. */
    float grToHeight (float grDB, const juce::Rectangle<float>& area) const noexcept;

    /** Iterate the history in display order and invoke a callback for each frame. */
    void forEachFrame (std::function<void(int col, const Frame&, int totalCols)> cb) const;

    void drawBackground    (juce::Graphics& g, const juce::Rectangle<float>& area) const;
    void drawCeilingLine   (juce::Graphics& g, const juce::Rectangle<float>& area,
                            const juce::Rectangle<float>& scaleArea) const;
    void drawOutputFill    (juce::Graphics& g, const juce::Rectangle<float>& area) const;
    void drawInputFill     (juce::Graphics& g, const juce::Rectangle<float>& area) const;
    void drawGainReduction  (juce::Graphics& g, const juce::Rectangle<float>& area) const;
    void drawOutputEnvelope (juce::Graphics& g, const juce::Rectangle<float>& area) const;
    void drawPeakMarkers    (juce::Graphics& g, const juce::Rectangle<float>& area) const;
    void drawScale          (juce::Graphics& g, const juce::Rectangle<float>& area) const;

    static constexpr float kScaleWidth = 30.0f;   // pixels reserved for dB scale
    static constexpr float kMaxGRdB    = 30.0f;   // full-scale GR (maps to top of display)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};
