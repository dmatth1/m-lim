#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Vertical stereo level meter with blue/yellow/red colour zones,
 * peak hold indicators, and smooth ballistics.
 *
 * Call setLevel() from the UI timer (~60fps) with smoothed dB values.
 * Call setPeakHold() to update the peak hold markers.
 */
class LevelMeter : public juce::Component
{
public:
    LevelMeter();
    ~LevelMeter() override = default;

    /** Set the current metered levels in dBFS. */
    void setLevel (float leftDB, float rightDB);

    /** Set peak hold values in dBFS (held externally, e.g. 2s then falls). */
    void setPeakHold (float leftDB, float rightDB);

    /** Reset peak hold markers. */
    void resetPeakHold();

    /** Latch clip indicators. Only cleared by resetClip() or a click. */
    void setClip (bool left, bool right);

    /** Reset both clip indicators. */
    void resetClip();

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;

private:
    static constexpr float kMinDB = -60.0f;
    static constexpr float kMaxDB =   0.0f;

    // Height reserved at the top of each bar for the numeric peak label
    static constexpr float kPeakLabelH = 14.0f;

    // dB thresholds for colour zones
    static constexpr float kWarnDB   = -3.0f;
    static constexpr float kDangerDB = -0.5f;

    float levelL_ = kMinDB;
    float levelR_ = kMinDB;
    float peakL_  = kMinDB;
    float peakR_  = kMinDB;
    bool  clipL_  = false;
    bool  clipR_  = false;

    /** Map a dB value to a normalised 0-1 position (0 = bottom, 1 = top). */
    static float dbToNorm (float db) noexcept;

    /** Draw one channel bar (including peak hold line and clip indicator). */
    void drawChannel (juce::Graphics& g,
                      juce::Rectangle<float> bar,
                      float levelDB,
                      float peakDB,
                      bool  clipped) const;

    /** Draw a numeric peak-hold label above a bar. */
    void drawPeakLabel (juce::Graphics& g,
                        const juce::Rectangle<float>& labelArea,
                        float peakDB) const;

    /** Draw dB scale labels on the right edge of the component. */
    void drawScale (juce::Graphics& g, float barTop, float barH) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};
