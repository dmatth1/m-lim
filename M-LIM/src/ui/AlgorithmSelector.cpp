#include "AlgorithmSelector.h"
#include "Colours.h"
#include "../dsp/LimiterAlgorithm.h"

static constexpr const char* kAlgorithmButtonLabels[] = {
    "Trans", "Punch", "Dyn", "Aggr", "Allrnd", "Bus", "Safe", "Modern"
};

AlgorithmSelector::AlgorithmSelector()
    : algoButtons_{ &transparentButton_, &punchyButton_, &dynamicButton_, &aggressiveButton_,
                    &allroundButton_,   &busButton_,    &safeButton_,    &modernButton_ }
{
    // Populate the hidden ComboBox (IDs are 1-based per JUCE convention).
    for (int i = 0; i < kNumAlgorithms; ++i)
        comboBox.addItem(kAlgorithmNames[i], i + 1);

    comboBox.setSelectedId(1, juce::dontSendNotification);

    // Route ComboBox changes (from APVTS or button clicks) to our callback
    // and keep button highlight states in sync.
    comboBox.onChange = [this]
    {
        updateButtonStates();
        if (onAlgorithmChanged)
            onAlgorithmChanged(comboBox.getSelectedId() - 1);
    };

    // The ComboBox is never visible — it's only here as an APVTS hook.
    addChildComponent(comboBox);

    // Configure each button and wire its click to the ComboBox.
    for (int i = 0; i < kNumAlgorithms; ++i)
    {
        algoButtons_[i]->setButtonText(kAlgorithmButtonLabels[i]);
        algoButtons_[i]->setClickingTogglesState(false);

        const int idx = i;
        algoButtons_[i]->onClick = [this, idx]
        {
            comboBox.setSelectedId(idx + 1, juce::sendNotificationSync);
        };

        addAndMakeVisible(*algoButtons_[i]);
    }

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
    const int selected = comboBox.getSelectedId() - 1;  // 0-based

    for (int i = 0; i < kNumAlgorithms; ++i)
    {
        const bool isSelected = (i == selected);

        algoButtons_[i]->setColour(juce::TextButton::buttonColourId,
                                   isSelected ? MLIMColours::accentBlue
                                              : MLIMColours::buttonBackground);
        algoButtons_[i]->setColour(juce::TextButton::buttonOnColourId,
                                   MLIMColours::accentBlue);
        algoButtons_[i]->setColour(juce::TextButton::textColourOffId,
                                   isSelected ? MLIMColours::textPrimary
                                              : MLIMColours::textSecondary);
        algoButtons_[i]->setColour(juce::TextButton::textColourOnId,
                                   MLIMColours::textPrimary);
    }
}

void AlgorithmSelector::paint(juce::Graphics& g)
{
    // Draw a subtle rounded background behind the whole component.
    auto bounds = getLocalBounds().toFloat();
    g.setColour(MLIMColours::displayBackground);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(MLIMColours::panelBorder);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
}

void AlgorithmSelector::resized()
{
    const int w = getWidth();
    const int h = getHeight();
    const int gap = 2;

    // Two rows of 4 buttons.
    const int rowH = (h - gap) / 2;
    const int colW = w / 4;

    for (int i = 0; i < kNumAlgorithms; ++i)
    {
        const int row = i / 4;
        const int col = i % 4;

        const int x = col * colW;
        const int y = row * (rowH + gap);
        const int bw = (col == 3) ? (w - x) : colW;  // last column takes the remainder

        algoButtons_[i]->setBounds(x, y, bw, rowH);
    }
}
