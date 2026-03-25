#include "AlgorithmSelector.h"
#include "Colours.h"

const char* const AlgorithmSelector::ALGORITHM_NAMES[AlgorithmSelector::NUM_ALGORITHMS] = {
    "Transparent",
    "Punchy",
    "Dynamic",
    "Aggressive",
    "Allround",
    "Bus",
    "Safe",
    "Modern"
};

AlgorithmSelector::AlgorithmSelector()
{
    for (int i = 0; i < NUM_ALGORITHMS; ++i)
        comboBox.addItem(ALGORITHM_NAMES[i], i + 1);  // ComboBox IDs are 1-based

    comboBox.setSelectedId(1, juce::dontSendNotification);

    comboBox.onChange = [this]
    {
        if (onAlgorithmChanged)
            onAlgorithmChanged(comboBox.getSelectedId() - 1);
    };

    // Style the combo box
    comboBox.setJustificationType(juce::Justification::centred);
    comboBox.setColour(juce::ComboBox::backgroundColourId,   MLIMColours::displayBackground);
    comboBox.setColour(juce::ComboBox::textColourId,         MLIMColours::textPrimary);
    comboBox.setColour(juce::ComboBox::outlineColourId,      MLIMColours::panelBorder);
    comboBox.setColour(juce::ComboBox::arrowColourId,        MLIMColours::textSecondary);
    comboBox.setColour(juce::ComboBox::focusedOutlineColourId, MLIMColours::accentBlue);

    addAndMakeVisible(comboBox);
}

void AlgorithmSelector::setAlgorithm(int index)
{
    if (index >= 0 && index < NUM_ALGORITHMS)
        comboBox.setSelectedId(index + 1, juce::sendNotificationSync);
}

int AlgorithmSelector::getAlgorithm() const
{
    return comboBox.getSelectedId() - 1;
}

void AlgorithmSelector::paint(juce::Graphics& g)
{
    // Draw rounded background behind the combo box
    auto bounds = getLocalBounds().toFloat();
    g.setColour(MLIMColours::displayBackground);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(MLIMColours::panelBorder);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
}

void AlgorithmSelector::resized()
{
    comboBox.setBounds(getLocalBounds());
}
