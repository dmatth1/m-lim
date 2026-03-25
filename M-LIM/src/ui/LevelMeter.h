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

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    static constexpr float kMinDB = -60.0f;
    static constexpr float kMaxDB =   0.0f;

    // dB thresholds for colour zones
    static constexpr float kWarnDB   = -3.0f;
    static constexpr float kDangerDB = -0.5f;

    float levelL_ = kMinDB;
    float levelR_ = kMinDB;
    float peakL_  = kMinDB;
    float peakR_  = kMinDB;

    /** Map a dB value to a normalised 0-1 position (0 = bottom, 1 = top). */
    static float dbToNorm (float db) noexcept;

    /** Draw one channel bar (including peak hold line) at the given rectangle. */
    void drawChannel (juce::Graphics& g,
                      juce::Rectangle<float> bar,
                      float levelDB,
                      float peakDB) const;

    /** Draw dB scale labels on the right edge of the component. */
    void drawScale (juce::Graphics& g) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};
