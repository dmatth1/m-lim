#include "PluginProcessor.h"
#include "PluginEditor.h"

MLIMAudioProcessor::MLIMAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

MLIMAudioProcessor::~MLIMAudioProcessor()
{
}

const juce::String MLIMAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MLIMAudioProcessor::acceptsMidi() const  { return false; }
bool MLIMAudioProcessor::producesMidi() const { return false; }
bool MLIMAudioProcessor::isMidiEffect() const { return false; }
double MLIMAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int MLIMAudioProcessor::getNumPrograms()                          { return 1; }
int MLIMAudioProcessor::getCurrentProgram()                       { return 0; }
void MLIMAudioProcessor::setCurrentProgram (int)                  {}
const juce::String MLIMAudioProcessor::getProgramName (int)       { return {}; }
void MLIMAudioProcessor::changeProgramName (int, const juce::String&) {}

void MLIMAudioProcessor::prepareToPlay (double /*sampleRate*/, int /*samplesPerBlock*/)
{
}

void MLIMAudioProcessor::releaseResources()
{
}

bool MLIMAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void MLIMAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    // Stub: pass audio through unchanged
}

bool MLIMAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* MLIMAudioProcessor::createEditor()
{
    return new MLIMAudioProcessorEditor (*this);
}

void MLIMAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr)
        copyXmlToBinary (*xml, destData);
}

void MLIMAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MLIMAudioProcessor();
}
