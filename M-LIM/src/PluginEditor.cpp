#include "PluginEditor.h"

MLIMAudioProcessorEditor::MLIMAudioProcessorEditor (MLIMAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (900, 500);
}

MLIMAudioProcessorEditor::~MLIMAudioProcessorEditor()
{
}

void MLIMAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a1a1a));
    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawFittedText ("M-LIM", getLocalBounds(), juce::Justification::centred, 1);
}

void MLIMAudioProcessorEditor::resized()
{
}
