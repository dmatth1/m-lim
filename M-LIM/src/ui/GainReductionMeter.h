#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * GainReductionMeter — vertical bar showing current GR and peak hold.
 *
 * The bar grows downward from the top (0 dB = no fill, -24 dB = full height).
 * A peak-hold tick and numeric readout are drawn below the bar.
 *
 * Call setGainReduction() and setPeakGR() from the 60 fps UI timer.
 * Call resetPeakGR() to clear the peak hold.
 */
class GainReductionMeter : public juce::Component
{
public:
    GainReductionMeter();
    ~GainReductionMeter() override = default;

    /** Set the current gain reduction (pass positive dB for reduction, e.g. 3.0 = 3 dB). */
    void setGainReduction (float dB);

    /** Set the peak-hold gain reduction value. */
    void setPeakGR (float dB);

    /** Reset the peak-hold value to 0. */
    void resetPeakGR();

    /** Set the full-scale range in dB (default 24 dB). */
    void setRange (float maxGRdB);

    void paint      (juce::Graphics& g) override;
    void resized    () override;
    void mouseDown  (const juce::MouseEvent& e) override;
    void mouseMove  (const juce::MouseEvent& e) override;
    void mouseExit  (const juce::MouseEvent& e) override;

private:
    /** Returns the rectangle used for the peak numeric readout. */
    juce::Rectangle<float> peakLabelArea() const;
    float currentGR_   = 0.0f;   // positive dB, 0 = no reduction
    float peakGR_      = 0.0f;   // positive dB
    float maxGRdB_     = 24.0f;  // full-scale range

    static constexpr int kScaleW  = 0;   // width of dB scale labels (0 = hidden)
    static constexpr int kNumericH = 0;  // numeric readout removed (was 16)

    void drawBar        (juce::Graphics& g, const juce::Rectangle<float>& barArea) const;
    void drawPeakTick   (juce::Graphics& g, const juce::Rectangle<float>& barArea) const;
    void drawScale      (juce::Graphics& g, const juce::Rectangle<float>& scaleArea) const;
    void drawNumeric    (juce::Graphics& g, const juce::Rectangle<float>& numArea) const;

    float grToFrac (float grDB) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GainReductionMeter)
};
