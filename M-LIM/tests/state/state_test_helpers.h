#pragma once

#include "catch2/catch_amalgamated.hpp"
#include <juce_audio_processors/juce_audio_processors.h>

// Minimal AudioProcessor used to host an APVTS in unit tests.
class StateTestProcessor : public juce::AudioProcessor
{
public:
    StateTestProcessor()
        : AudioProcessor(BusesProperties()
              .withInput("Input",   juce::AudioChannelSet::stereo())
              .withOutput("Output", juce::AudioChannelSet::stereo())),
          apvts(*this, nullptr, "State", createLayout())
    {}

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "gain", "Gain", -60.0f, 12.0f, 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "ceiling", "Ceiling", -30.0f, 0.0f, -0.1f));
        return layout;
    }

    juce::AudioProcessorValueTreeState apvts;

    // Required AudioProcessor overrides
    const juce::String getName() const override { return "StateTestProcessor"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    void processBlock(juce::AudioBuffer<double>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}
};

// Helper to get a float parameter value from the APVTS
static float getParam(juce::AudioProcessorValueTreeState& apvts, const char* id)
{
    auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(id));
    REQUIRE(param != nullptr);
    return param->get();
}

// Helper to set a float parameter value
static void setParam(juce::AudioProcessorValueTreeState& apvts, const char* id, float value)
{
    auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter(id));
    REQUIRE(param != nullptr);
    *param = value;
}

// RAII temporary directory for test presets.
struct TempPresetDir
{
    juce::File dir;

    TempPresetDir()
    {
        dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                  .getChildFile("MLIMPresetTest_"
                      + juce::String(juce::Random::getSystemRandom().nextInt64()));
        dir.createDirectory();
    }

    ~TempPresetDir()
    {
        dir.deleteRecursively();
    }
};
