#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <functional>
#include <cmath>

/**
 * LoudnessPanel — Pro-L 2 style loudness histogram + numeric readouts.
 *
 * Top area: loudness distribution histogram
 *   - Horizontal bars at each dB level (-35 to 0 LUFS, 0.5 dB steps)
 *   - Bar width = time accumulated at that loudness level
 *   - Color-coded relative to target LUFS
 *   - dB scale labels on the left
 *   - Target level highlighted with distinct background
 *
 * Bottom area: numeric readout rows
 *   Momentary LUFS   [value] [bar]
 *   Short-Term LUFS  [value] [bar]
 *   Integrated LUFS  [value] [bar] [Reset]
 *   Range LU         [value]
 *   True Peak dBTP   [value]
 *
 * Call setMomentary/setShortTerm/setIntegrated/... from the 60 fps UI timer.
 * Call pushLoudnessData() with the active LUFS reading each frame to accumulate
 * the histogram.
 */
class LoudnessPanel : public juce::Component
{
public:
    LoudnessPanel();
    ~LoudnessPanel() override = default;

    // ── Data setters (call from UI timer) ─────────────────────────────────
    void setMomentary     (float lufs);
    void setShortTerm     (float lufs);
    void setIntegrated    (float lufs);
    void setLoudnessRange (float lu);
    void setTruePeak      (float dBTP);

    /** Set the target LUFS reference line on bar meters (default -14 LUFS). */
    void setTarget (float lufs);

    /**
     * Push a loudness reading to accumulate the histogram.
     * Call once per UI frame with the current short-term or integrated LUFS.
     */
    void pushLoudnessData (float lufs);

    /** Clear all histogram bins and reset the display. */
    void resetHistogram();

    /** Called when the user clicks the integrated-LUFS Reset button. */
    std::function<void()> onResetIntegrated;

    /**
     * Called when the user selects a new target from the popup menu.
     * Argument is the choice index (0-3 = standard targets, 4 = custom).
     * For custom, the LUFS value is already applied via setTarget().
     */
    std::function<void(int)> onTargetChanged;

    /**
     * Set the active target choice (0-3 = standard, 4 = custom).
     * Updates the displayed label and target LUFS line.
     */
    void setTargetChoice (int choiceIndex);

    /** Returns the currently active target choice index. */
    int getTargetChoice() const noexcept { return targetChoice_; }

    /** Returns the short display label for choice index (e.g. "-14 (Strm)"). */
    static juce::String targetChoiceLabel (int choiceIndex) noexcept;

    /** Returns the LUFS value for a standard target choice (0-3). */
    static float targetChoiceToLUFS (int choiceIndex) noexcept;

    // Component overrides
    void paint   (juce::Graphics& g) override;
    void resized () override;

private:
    // ── Loudness state ────────────────────────────────────────────────────
    float momentary_     = -std::numeric_limits<float>::infinity();
    float shortTerm_     = -std::numeric_limits<float>::infinity();
    float integrated_    = -std::numeric_limits<float>::infinity();
    float loudnessRange_ = 0.0f;
    float truePeak_      = -std::numeric_limits<float>::infinity();
    float targetLUFS_    = -14.0f;

    // ── Histogram state ───────────────────────────────────────────────────
    static constexpr int   kHistBins = 70;       // -35 to 0 LUFS in 0.5 dB steps
    static constexpr float kHistMin  = -35.0f;
    static constexpr float kHistStep = 0.5f;

    std::array<float, kHistBins> histogramBins_ {};
    float histogramMax_ = 1.0f;

    // ── Target selector state ─────────────────────────────────────────────
    int   targetChoice_     = 1;         // default: Streaming (-14 LUFS)
    float customTargetLUFS_ = -14.0f;   // used when targetChoice_ == 4

    // ── Children ──────────────────────────────────────────────────────────
    juce::TextButton resetButton_  { "RST" };
    juce::TextButton targetButton_;

    // ── Owned modal dialog (RAII lifetime, reset in callback or destructor) ─
    std::unique_ptr<juce::AlertWindow> customAlertWindow_;

    // ── Layout constants ──────────────────────────────────────────────────
    static constexpr int kRowH     = 22;
    static constexpr int kPadding  = 4;
    static constexpr int kLabelW   = 72;
    static constexpr int kValueW   = 52;
    static constexpr int kBtnW     = 28;
    static constexpr float kMinLUFS = -60.0f;

    /** Height reserved for the numeric readout rows at the bottom. */
    static constexpr int kReadoutAreaH = kPadding + 5 * kRowH + kPadding;

    // ── Private drawing helpers ───────────────────────────────────────────

    /** Draw the loudness distribution histogram in the given bounds. */
    void drawHistogram (juce::Graphics& g, juce::Rectangle<int> bounds) const;

    /** Draw one readout row: label / value / optional bar. */
    void drawRow (juce::Graphics& g,
                  const juce::Rectangle<int>& rowBounds,
                  const juce::String& label,
                  const juce::String& valueStr,
                  float  barFill,
                  bool   showTarget,
                  float  targetFrac) const;

    /** Draw a horizontal bar meter. */
    void drawBar (juce::Graphics& g,
                  const juce::Rectangle<float>& barBounds,
                  float fill,
                  bool  showTarget,
                  float targetFrac) const;

    /** Normalise a LUFS value to 0-1 for bar display (-60 → 0, 0 → 1). */
    static float lufsToFrac (float lufs) noexcept;

    /** Format a LUFS value as a string, e.g. "-14.2". Returns "---" for -inf. */
    static juce::String fmtLUFS (float lufs);

    /** Format a dBTP value. */
    static juce::String fmtDBTP (float dBTP);

    /** Return the histogram bar colour for a given LUFS level vs target. */
    static juce::Colour histogramBarColour (float binLUFS, float targetLUFS) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoudnessPanel)
};
