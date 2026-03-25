#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * LoudnessPanel — shows ITU-R BS.1770-4 loudness readings with bar meters.
 *
 * Layout (vertical stack):
 *   Momentary LUFS   [value] [bar]
 *   Short-Term LUFS  [value] [bar]
 *   Integrated LUFS  [value] [bar] [Reset]
 *   Range LU         [value]
 *   True Peak dBTP   [value]
 *
 * Call the set*() methods from the 60 fps UI timer after draining the FIFO.
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

    /** Called when the user clicks the integrated-LUFS Reset button. */
    std::function<void()> onResetIntegrated;

    // Component overrides
    void paint   (juce::Graphics& g) override;
    void resized () override;

private:
    float momentary_     = -std::numeric_limits<float>::infinity();
    float shortTerm_     = -std::numeric_limits<float>::infinity();
    float integrated_    = -std::numeric_limits<float>::infinity();
    float loudnessRange_ = 0.0f;
    float truePeak_      = -std::numeric_limits<float>::infinity();
    float targetLUFS_    = -14.0f;

    juce::TextButton resetButton_ { "RST" };

    /** Draw one row: label on left, formatted value in middle, optional bar on right. */
    void drawRow (juce::Graphics& g,
                  const juce::Rectangle<int>& rowBounds,
                  const juce::String& label,
                  const juce::String& valueStr,
                  float  barFill,        ///< 0-1 normalised fill, <0 means no bar
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

    static constexpr int kRowH     = 22;
    static constexpr int kPadding  = 4;
    static constexpr int kLabelW   = 72;
    static constexpr int kValueW   = 52;
    static constexpr int kBtnW     = 28;
    static constexpr float kMinLUFS = -60.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoudnessPanel)
};
