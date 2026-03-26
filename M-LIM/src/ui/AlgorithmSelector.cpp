#include "AlgorithmSelector.h"
#include "Colours.h"
#include "../dsp/LimiterAlgorithm.h"

// Short display names for the compact navigation widget (ALL CAPS).
static constexpr const char* kAlgorithmButtonLabels[] = {
    "TRANSPARENT", "PUNCHY", "DYNAMIC", "AGGRESSIVE",
    "ALLROUND", "BUS", "SAFE", "MODERN"
};

AlgorithmSelector::AlgorithmSelector()
{
    // Populate the hidden ComboBox (IDs are 1-based per JUCE convention).
    for (int i = 0; i < kNumAlgorithms; ++i)
        comboBox.addItem(kAlgorithmNames[i], i + 1);

    comboBox.setSelectedId(1, juce::dontSendNotification);

    // Route ComboBox changes (from APVTS or button clicks) to our callback.
    comboBox.onChange = [this]
    {
        updateButtonStates();
        if (onAlgorithmChanged)
            onAlgorithmChanged(comboBox.getSelectedId() - 1);
    };

    // The ComboBox is never visible — only here as an APVTS hook.
    addChildComponent(comboBox);

    // Prev button: cycle backwards.
    prevButton_.setButtonText(juce::CharPointer_UTF8("\xe2\x80\xb9"));  // ‹
    prevButton_.onClick = [this]
    {
        const int cur  = comboBox.getSelectedId() - 1;
        const int next = (cur - 1 + kNumAlgorithms) % kNumAlgorithms;
        comboBox.setSelectedId(next + 1, juce::sendNotificationSync);
    };

    // Next button: cycle forwards.
    nextButton_.setButtonText(juce::CharPointer_UTF8("\xe2\x80\xba"));  // ›
    nextButton_.onClick = [this]
    {
        const int cur  = comboBox.getSelectedId() - 1;
        const int next = (cur + 1) % kNumAlgorithms;
        comboBox.setSelectedId(next + 1, juce::sendNotificationSync);
    };

    // Style the nav buttons.
    for (auto* btn : { &prevButton_, &nextButton_ })
    {
        btn->setColour(juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
        btn->setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        btn->setColour(juce::TextButton::textColourOffId,  MLIMColours::textPrimary);
        btn->setColour(juce::TextButton::textColourOnId,   MLIMColours::textPrimary);
        addAndMakeVisible(*btn);
    }

    // Style the name label.
    nameLabel_.setFont(juce::Font(MLIMColours::kFontSizeMedium, juce::Font::bold));
    nameLabel_.setColour(juce::Label::textColourId, MLIMColours::textPrimary);
    nameLabel_.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    nameLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(nameLabel_);

    updateButtonStates();
}

void AlgorithmSelector::setAlgorithm(int index)
{
    if (index >= 0 && index < kNumAlgorithms)
        comboBox.setSelectedId(index + 1, juce::sendNotificationSync);
}

int AlgorithmSelector::getAlgorithm() const
{
    return comboBox.getSelectedId() - 1;
}

void AlgorithmSelector::updateButtonStates()
{
    const int selected = juce::jlimit(0, kNumAlgorithms - 1, comboBox.getSelectedId() - 1);
    nameLabel_.setText(kAlgorithmButtonLabels[selected], juce::dontSendNotification);
}

void AlgorithmSelector::paint(juce::Graphics& g)
{
    // Subtle rounded background behind the whole component.
    auto bounds = getLocalBounds().toFloat();
    g.setColour(MLIMColours::displayBackground);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(MLIMColours::algoButtonInactive.brighter(0.3f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
}

void AlgorithmSelector::resized()
{
    auto b = getLocalBounds();
    prevButton_.setBounds(b.removeFromLeft(20));
    nextButton_.setBounds(b.removeFromRight(20));
    nameLabel_.setBounds(b);
}
