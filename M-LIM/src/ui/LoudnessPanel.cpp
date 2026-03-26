#include "LoudnessPanel.h"
#include "Colours.h"
#include <cmath>
#include <algorithm>
#include <limits>

// ─────────────────────────────────────────────────────────────────────────────

LoudnessPanel::LoudnessPanel()
{
    resetButton_.setClickingTogglesState (false);
    resetButton_.setColour (juce::TextButton::buttonColourId,  MLIMColours::widgetBackground);
    resetButton_.setColour (juce::TextButton::textColourOffId, MLIMColours::textSecondary);
    resetButton_.onClick = [this]
    {
        if (onResetIntegrated) onResetIntegrated();
        resetHistogram();
    };
    addAndMakeVisible (resetButton_);

    // Target selector button
    targetButton_.setButtonText (targetChoiceLabel (targetChoice_));
    targetButton_.setClickingTogglesState (false);
    targetButton_.setColour (juce::TextButton::buttonColourId,  MLIMColours::accentDarkBackground);
    targetButton_.setColour (juce::TextButton::textColourOffId, MLIMColours::accentBlue);
    targetButton_.onClick = [this]
    {
        juce::PopupMenu menu;
        menu.addItem (1, "-9 LUFS (CD)");
        menu.addItem (2, "-14 LUFS (Streaming)");
        menu.addItem (3, "-23 LUFS (EBU R128)");
        menu.addItem (4, "-24 LUFS (ATSC A/85)");
        menu.addSeparator();
        menu.addItem (5, "Custom...");

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&targetButton_),
            [this] (int result)
            {
                if (result == 0) return;

                const int choiceIndex = result - 1;   // result 1..5 → index 0..4

                if (choiceIndex == 4)
                {
                    // Custom: ask for a value via AlertWindow
                    // Lifetime is managed by customAlertWindow_ (RAII); SafePointer
                    // guards against the component being destroyed before callback fires.
                    juce::Component::SafePointer<LoudnessPanel> safeThis (this);

                    customAlertWindow_ = std::make_unique<juce::AlertWindow> (
                        "Custom Target",
                        "Enter target LUFS (e.g. -16):",
                        juce::MessageBoxIconType::NoIcon);
                    customAlertWindow_->addTextEditor ("value",
                                                       juce::String (customTargetLUFS_, 1),
                                                       "LUFS:");
                    customAlertWindow_->addButton ("OK",     1, juce::KeyPress (juce::KeyPress::returnKey));
                    customAlertWindow_->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

                    customAlertWindow_->enterModalState (true,
                        juce::ModalCallbackFunction::create ([safeThis] (int r)
                        {
                            if (safeThis == nullptr) return;
                            if (r == 1)
                            {
                                const float val = safeThis->customAlertWindow_
                                                      ->getTextEditorContents ("value")
                                                      .getFloatValue();
                                safeThis->customTargetLUFS_ = juce::jlimit (-60.0f, 0.0f, val);
                                safeThis->targetChoice_ = 4;
                                safeThis->setTarget (safeThis->customTargetLUFS_);
                                safeThis->targetButton_.setButtonText (
                                    juce::String (safeThis->customTargetLUFS_, 1));
                                if (safeThis->onTargetChanged) safeThis->onTargetChanged (4);
                            }
                            safeThis->customAlertWindow_.reset();
                        }),
                        true);
                    return;
                }

                setTargetChoice (choiceIndex);
                if (onTargetChanged) onTargetChanged (choiceIndex);
            });
    };
    addAndMakeVisible (targetButton_);
}

// ── Data setters ─────────────────────────────────────────────────────────────

void LoudnessPanel::setMomentary     (float lufs) { momentary_     = lufs; repaint(); }
void LoudnessPanel::setShortTerm     (float lufs) { shortTerm_     = lufs; repaint(); }
void LoudnessPanel::setIntegrated    (float lufs) { integrated_    = lufs; repaint(); }
void LoudnessPanel::setLoudnessRange (float lu)   { loudnessRange_ = lu;   repaint(); }
void LoudnessPanel::setTruePeak      (float dBTP) { truePeak_      = dBTP; repaint(); }
void LoudnessPanel::setTarget        (float lufs) { targetLUFS_    = lufs; repaint(); }

// ── Target choice helpers ─────────────────────────────────────────────────────

float LoudnessPanel::targetChoiceToLUFS (int choiceIndex) noexcept
{
    static constexpr float kTargetLUFS[] = { -9.0f, -14.0f, -23.0f, -24.0f };
    static constexpr int   kNumTargets   = (int) std::size (kTargetLUFS);

    jassert (choiceIndex >= 0 && choiceIndex < kNumTargets);
    return kTargetLUFS[juce::jlimit (0, kNumTargets - 1, choiceIndex)];
}

juce::String LoudnessPanel::targetChoiceLabel (int choiceIndex) noexcept
{
    static constexpr const char* kTargetLabels[] = {
        "-9 (CD)", "-14 (Strm)", "-23 (EBU)", "-24 (ATSC)"
    };
    static constexpr int kNumTargets = (int) std::size (kTargetLabels);

    if (choiceIndex < 0 || choiceIndex >= kNumTargets)
        return "Custom";
    return kTargetLabels[choiceIndex];
}

void LoudnessPanel::setTargetChoice (int choiceIndex)
{
    targetChoice_ = choiceIndex;
    if (choiceIndex < 4)
    {
        const float lufs = targetChoiceToLUFS (choiceIndex);
        setTarget (lufs);
        targetButton_.setButtonText (targetChoiceLabel (choiceIndex));
    }
    // For custom (index 4), the button text is set by the caller after entering value
}

// ── Histogram accumulation ────────────────────────────────────────────────────

void LoudnessPanel::pushLoudnessData (float lufs)
{
    if (std::isinf (lufs) || std::isnan (lufs)
        || lufs < kHistMin || lufs > 0.0f)
        return;

    int bin = juce::jlimit (0, kHistBins - 1,
                            static_cast<int> ((lufs - kHistMin) / kHistStep));
    histogramBins_[static_cast<size_t> (bin)] += 1.0f;

    histogramMax_ = *std::max_element (histogramBins_.begin(), histogramBins_.end());
    repaint();
}

void LoudnessPanel::resetHistogram()
{
    histogramBins_.fill (0.0f);
    histogramMax_ = 1.0f;
    repaint();
}

// ─────────────────────────────────────────────────────────────────────────────

void LoudnessPanel::resized()
{
    const int histH  = std::max (0, getHeight() - kReadoutAreaH);
    // Integrated row = row index 2 within the readout area
    const int rowY   = histH + kPadding + kRowH * 2;

    resetButton_.setBounds (getWidth() - kBtnW - kPadding,
                            rowY + (kRowH - 18) / 2,
                            kBtnW,
                            18);

    // Target selector button: top-right corner of histogram area
    constexpr int kTargetBtnH = 16;
    constexpr int kTargetBtnW = 66;
    targetButton_.setBounds (getWidth() - kTargetBtnW - kPadding,
                             kPadding,
                             kTargetBtnW,
                             kTargetBtnH);
}

// ─────────────────────────────────────────────────────────────────────────────

void LoudnessPanel::paint (juce::Graphics& g)
{
    // Panel background
    g.setColour (MLIMColours::displayBackground);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 4.0f);

    g.setColour (MLIMColours::panelBorder);
    g.drawRoundedRectangle (getLocalBounds().reduced (1).toFloat(), 4.0f, 1.0f);

    // ── Histogram ────────────────────────────────────────────────────────
    const int histH = std::max (0, getHeight() - kReadoutAreaH);
    if (histH > 30)
    {
        auto histBounds = getLocalBounds().withHeight (histH);
        drawHistogram (g, histBounds);

        // Separator line between histogram and readout rows
        g.setColour (MLIMColours::panelBorder);
        g.drawHorizontalLine (histH, 2.0f, static_cast<float> (getWidth() - 2));
    }

    // ── Numeric readout rows ──────────────────────────────────────────────
    const int readoutTop = histH;
    const float targetFrac = lufsToFrac (targetLUFS_);

    // Row 0: Momentary
    drawRow (g,
             getLocalBounds().withTrimmedTop (readoutTop + kPadding + kRowH * 0).withHeight (kRowH),
             "Momentary",
             fmtLUFS (momentary_),
             lufsToFrac (momentary_),
             true,
             targetFrac);

    // Row 1: Short-Term
    drawRow (g,
             getLocalBounds().withTrimmedTop (readoutTop + kPadding + kRowH * 1).withHeight (kRowH),
             "Short-Term",
             fmtLUFS (shortTerm_),
             lufsToFrac (shortTerm_),
             true,
             targetFrac);

    // Row 2: Integrated (reset button is a child component)
    drawRow (g,
             getLocalBounds().withTrimmedTop (readoutTop + kPadding + kRowH * 2).withHeight (kRowH),
             "Integrated",
             fmtLUFS (integrated_),
             lufsToFrac (integrated_),
             true,
             targetFrac);

    // Row 3: Range (no bar)
    {
        juce::String val = (std::isinf (loudnessRange_) || loudnessRange_ <= 0.0f)
                         ? "---"
                         : juce::String (loudnessRange_, 1);
        drawRow (g,
                 getLocalBounds().withTrimmedTop (readoutTop + kPadding + kRowH * 3).withHeight (kRowH),
                 "Range",
                 val + " LU",
                 -1.0f,
                 false,
                 0.0f);
    }

    // Row 4: True Peak (no bar)
    drawRow (g,
             getLocalBounds().withTrimmedTop (readoutTop + kPadding + kRowH * 4).withHeight (kRowH),
             "True Peak",
             fmtDBTP (truePeak_),
             -1.0f,
             false,
             0.0f);

    // ── Large LUFS readout ────────────────────────────────────────────────
    const int largeTop = readoutTop + kPadding + 5 * kRowH + kPadding;
    if (getHeight() > largeTop + 10)
    {
        g.setColour (MLIMColours::panelBorder);
        g.drawHorizontalLine (largeTop, 2.0f, static_cast<float> (getWidth() - 2));

        drawLargeReadout (g, getLocalBounds().withTrimmedTop (largeTop)
                                             .withHeight (kLargeReadoutH));
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void LoudnessPanel::drawHistogram (juce::Graphics& g,
                                   juce::Rectangle<int> bounds) const
{
    // Layout: left strip = dB scale labels, right area = histogram bars
    constexpr int kScaleW = 26;
    constexpr int kBarPad = 3;   // padding around bars area

    const auto scaleRect = bounds.withWidth (kScaleW);
    const auto barsArea  = bounds.withTrimmedLeft (kScaleW).reduced (kBarPad, kBarPad);

    const float totalH    = static_cast<float> (barsArea.getHeight());
    const float barH      = totalH / static_cast<float> (kHistBins);
    const float maxVal    = std::max (histogramMax_, 1.0f);
    const float barAreaW  = static_cast<float> (barsArea.getWidth());
    const float originX   = static_cast<float> (barsArea.getX());
    const float originY   = static_cast<float> (barsArea.getY());

    // ── Draw histogram bars (bin 0 = -35 LUFS at bottom, bin 69 = 0 LUFS at top) ──
    for (int i = 0; i < kHistBins; ++i)
    {
        const float binLUFS = kHistMin + static_cast<float> (i) * kHistStep;

        // Map bin index to Y: higher LUFS (louder) = higher on screen (smaller Y)
        // bin kHistBins-1 (0 LUFS) maps to top, bin 0 (-35 LUFS) maps to bottom
        const float normPos = static_cast<float> (i) / static_cast<float> (kHistBins);
        const float barY    = originY + totalH - (normPos + 1.0f / kHistBins) * totalH;

        // Target level highlight (slightly lighter background)
        if (std::abs (binLUFS - targetLUFS_) < kHistStep * 0.5f)
        {
            g.setColour (MLIMColours::histogramHighlight);
            g.fillRect (juce::Rectangle<float> (originX, barY, barAreaW, barH));
        }

        const float count    = histogramBins_[static_cast<size_t> (i)];
        const float barWidth = (count / maxVal) * barAreaW;

        if (barWidth >= 0.5f)
        {
            g.setColour (histogramBarColour (binLUFS, targetLUFS_));
            g.fillRect (juce::Rectangle<float> (originX, barY + 0.5f,
                                                barWidth,
                                                std::max (barH - 1.0f, 0.5f)));
        }
    }

    // ── Draw target indicator line ────────────────────────────────────────
    {
        const float normTarget = (targetLUFS_ - kHistMin) / (kHistBins * kHistStep);
        const float targetY    = originY + totalH - normTarget * totalH;

        g.setColour (MLIMColours::peakLabel.withAlpha (0.85f));
        g.drawHorizontalLine (juce::roundToInt (targetY),
                              originX,
                              originX + barAreaW);

        // Target label on the scale side
        juce::String targetStr = juce::String (static_cast<int> (std::round (targetLUFS_)));
        g.setFont (juce::Font (8.5f, juce::Font::bold));
        g.setColour (MLIMColours::peakLabel);
        g.drawText (targetStr,
                    scaleRect.withY (juce::roundToInt (targetY) - 6).withHeight (12),
                    juce::Justification::centredRight,
                    false);
    }

    // ── dB scale labels ───────────────────────────────────────────────────
    // Draw labels every 5 dB: 0, -5, -10, -15, -20, -25, -30, -35
    g.setFont (juce::Font (8.0f));
    g.setColour (MLIMColours::textSecondary);

    for (int dB = 0; dB >= -35; dB -= 5)
    {
        const float normPos = (static_cast<float> (dB) - kHistMin) / (kHistBins * kHistStep);
        const float labelY  = originY + totalH - normPos * totalH;

        // Skip if too close to the target label
        if (std::abs (static_cast<float> (dB) - targetLUFS_) < 3.0f)
            continue;

        juce::String label = (dB == 0) ? "0" : juce::String (dB);
        g.drawText (label,
                    scaleRect.withY (juce::roundToInt (labelY) - 6).withHeight (12),
                    juce::Justification::centredRight,
                    false);

        // Tick mark
        g.setColour (MLIMColours::panelBorder);
        g.drawHorizontalLine (juce::roundToInt (labelY),
                              originX,
                              originX + 3.0f);
        g.setColour (MLIMColours::textSecondary);
    }

    // ── "LUFS" header label ───────────────────────────────────────────────
    g.setFont (juce::Font (8.0f));
    g.setColour (MLIMColours::textSecondary.withAlpha (0.7f));
    g.drawText ("LUFS",
                bounds.withHeight (12).withY (bounds.getY() + 2),
                juce::Justification::centred,
                false);
}

// ─────────────────────────────────────────────────────────────────────────────

juce::Colour LoudnessPanel::histogramBarColour (float binLUFS,
                                                float targetLUFS) noexcept
{
    const float diff = binLUFS - targetLUFS;   // positive = louder than target

    if (diff >= 0.0f)
        return MLIMColours::meterWarning;                         // above target: yellow
    else if (diff >= -2.0f)
        return MLIMColours::meterAtTarget;                        // at target ±2: orange
    else if (diff >= -6.0f)
        return MLIMColours::lufsReadoutGood;                      // approaching: warm golden
    else
        return MLIMColours::lufsReadoutGood.withAlpha (0.65f);    // below target: warm golden (dimmer)
}

// ─────────────────────────────────────────────────────────────────────────────

void LoudnessPanel::drawLargeReadout (juce::Graphics& g,
                                      juce::Rectangle<int> bounds) const
{
    const juce::String valStr = fmtLUFS (shortTerm_);
    juce::Colour valColour;
    if (shortTerm_ >= targetLUFS_)
        valColour = MLIMColours::meterDanger;                         // over target: red
    else if (shortTerm_ >= targetLUFS_ - 1.0f)
        valColour = MLIMColours::meterWarning;                        // within 1 LU of target: yellow
    else
        valColour = MLIMColours::lufsReadoutGood;                     // comfortably below: golden

    // Value in large bold font — occupies the upper 2/3 of the strip
    g.setFont (juce::Font (28.0f, juce::Font::bold));
    g.setColour (valColour);
    g.drawText (valStr,
                bounds.withTrimmedBottom (bounds.getHeight() / 3),
                juce::Justification::centred,
                false);

    // "LUFS" unit label in smaller font — occupies the lower 1/3
    g.setFont (juce::Font (MLIMColours::kFontSizeMedium));
    g.setColour (MLIMColours::textSecondary);
    g.drawText ("LUFS",
                bounds.withTrimmedTop (bounds.getHeight() * 2 / 3),
                juce::Justification::centred,
                false);
}

// ─────────────────────────────────────────────────────────────────────────────

void LoudnessPanel::drawRow (juce::Graphics& g,
                              const juce::Rectangle<int>& rowBounds,
                              const juce::String& label,
                              const juce::String& valueStr,
                              float  barFill,
                              bool   showTarget,
                              float  targetFrac) const
{
    auto r = rowBounds.reduced (kPadding, 2);

    // Label
    g.setColour (MLIMColours::textSecondary);
    g.setFont (juce::Font (MLIMColours::kFontSizeMedium));
    g.drawText (label, r.removeFromLeft (kLabelW), juce::Justification::centredLeft, false);

    // Value
    g.setColour (MLIMColours::textPrimary);
    g.setFont (juce::Font (MLIMColours::kFontSizeLarge, juce::Font::bold));
    g.drawText (valueStr, r.removeFromLeft (kValueW), juce::Justification::centredRight, false);

    // Bar (optional)
    if (barFill >= 0.0f)
    {
        r.removeFromLeft (4);
        drawBar (g, r.toFloat(), barFill, showTarget, targetFrac);
    }
}

void LoudnessPanel::drawBar (juce::Graphics& g,
                              const juce::Rectangle<float>& barBounds,
                              float fill,
                              bool  showTarget,
                              float targetFrac) const
{
    // Background track
    g.setColour (MLIMColours::barTrackBackground);
    g.fillRoundedRectangle (barBounds, 2.0f);

    if (fill <= 0.0f) return;

    float fillW = juce::jlimit (0.0f, barBounds.getWidth(), fill * barBounds.getWidth());
    auto filled = barBounds.withWidth (fillW);

    // Color zones
    juce::Colour barColour;
    if (fill < 0.70f)
        barColour = MLIMColours::meterSafe;
    else if (fill < 0.90f)
        barColour = MLIMColours::meterWarning;
    else
        barColour = MLIMColours::meterDanger;

    g.setColour (barColour);
    g.fillRoundedRectangle (filled, 2.0f);

    // Target reference line
    if (showTarget && targetFrac > 0.0f && targetFrac < 1.0f)
    {
        float tx = barBounds.getX() + targetFrac * barBounds.getWidth();
        g.setColour (MLIMColours::peakLabel.withAlpha (0.8f));
        g.drawVerticalLine (juce::roundToInt (tx), barBounds.getY(), barBounds.getBottom());
    }
}

// ─────────────────────────────────────────────────────────────────────────────

float LoudnessPanel::lufsToFrac (float lufs) noexcept
{
    if (std::isinf (lufs) || std::isnan (lufs) || lufs < kMinLUFS)
        return 0.0f;
    return juce::jlimit (0.0f, 1.0f, (lufs - kMinLUFS) / (-kMinLUFS));
}

juce::String LoudnessPanel::fmtLUFS (float lufs)
{
    if (std::isinf (lufs) || std::isnan (lufs) || lufs < -200.0f)
        return "---";
    return juce::String (lufs, 1);
}

juce::String LoudnessPanel::fmtDBTP (float dBTP)
{
    if (std::isinf (dBTP) || std::isnan (dBTP) || dBTP < -200.0f)
        return "---";
    return juce::String (dBTP, 1) + " dBTP";
}
