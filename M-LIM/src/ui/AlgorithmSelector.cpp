#include "AlgorithmSelector.h"
#include "Colours.h"
#include "../dsp/LimiterAlgorithm.h"

// Short display names for the compact button grid (ALL CAPS).
static constexpr const char* kAlgorithmButtonLabels[] = {
    "TR", "PU", "DY", "AG",
    "AR", "BU", "SA", "MO"
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

    // Build 2×4 algorithm button grid.
    for (int i = 0; i < kNumAlgorithms; ++i)
    {
        algoButtons_[i].setButtonText(kAlgorithmButtonLabels[i]);
        algoButtons_[i].setClickingTogglesState(false);
        algoButtons_[i].setColour(juce::TextButton::buttonColourId,   MLIMColours::algoButtonInactive);
        algoButtons_[i].setColour(juce::TextButton::buttonOnColourId, MLIMColours::algoButtonSelected);
        algoButtons_[i].setColour(juce::TextButton::textColourOffId,  MLIMColours::textSecondary);
        algoButtons_[i].setColour(juce::TextButton::textColourOnId,   MLIMColours::textPrimary);
        algoButtons_[i].onClick = [this, i]
        {
            comboBox.setSelectedId(i + 1, juce::sendNotificationSync);
        };
        addAndMakeVisible(algoButtons_[i]);
    }

    // Hide legacy nav widgets.
    prevButton_.setVisible(false);
    nextButton_.setVisible(false);
    nameLabel_.setVisible(false);

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
    for (int i = 0; i < kNumAlgorithms; ++i)
        algoButtons_[i].setToggleState(i == selected, juce::dontSendNotification);
    nameLabel_.setText(kAlgorithmButtonLabels[selected], juce::dontSendNotification);
}

void AlgorithmSelector::paint(juce::Graphics& g)
{
    // Subtle rounded background behind the button grid.
    auto bounds = getLocalBounds().toFloat();
    g.setColour(MLIMColours::algoButtonInactive);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(MLIMColours::algoButtonInactive.brighter(0.3f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
}

void AlgorithmSelector::resized()
{
    auto b = getLocalBounds();
    const int rowH = b.getHeight() / 2;
    const int colW = b.getWidth() / 4;
    for (int i = 0; i < kNumAlgorithms; ++i)
    {
        int row = i / 4;
        int col = i % 4;
        algoButtons_[i].setBounds(col * colW, row * rowH, colW, rowH);
    }
}
