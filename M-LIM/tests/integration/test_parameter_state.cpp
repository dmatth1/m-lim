/**
 * test_parameter_state.cpp — APVTS parameter layout integration tests (Task 041)
 *
 * Verifies that all sidechain filter and display-mode parameters are present
 * in the APVTS layout returned by createParameterLayout(), using a minimal
 * stub AudioProcessor as the required APVTS owner.
 */
#include "catch2/catch_amalgamated.hpp"
#include "Parameters.h"
#include <juce_audio_processors/juce_audio_processors.h>

// ---------------------------------------------------------------------------
// Minimal stub processor — satisfies APVTS ownership requirements
// ---------------------------------------------------------------------------
class StubProcessor : public juce::AudioProcessor
{
public:
    StubProcessor()
        : AudioProcessor (BusesProperties()
              .withInput  ("Input",  juce::AudioChannelSet::stereo())
              .withOutput ("Output", juce::AudioChannelSet::stereo()))
    {}

    const juce::String getName() const override { return "Stub"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}
};

// ---------------------------------------------------------------------------
// Helper: build an APVTS with the full parameter layout
// ---------------------------------------------------------------------------
static juce::AudioProcessorValueTreeState makeAPVTS (juce::AudioProcessor& owner)
{
    return juce::AudioProcessorValueTreeState (
        owner, nullptr, "Parameters", createParameterLayout());
}

// ============================================================================
// test_sidechain_params_exist
// Verify that sidechainHPFreq, sidechainLPFreq, and sidechainTilt are all
// registered in the APVTS layout.
// ============================================================================
TEST_CASE("test_sidechain_params_exist", "[ParameterState]")
{
    StubProcessor proc;
    auto apvts = makeAPVTS (proc);

    REQUIRE (apvts.getParameter (ParamID::sidechainHPFreq) != nullptr);
    REQUIRE (apvts.getParameter (ParamID::sidechainLPFreq) != nullptr);
    REQUIRE (apvts.getParameter (ParamID::sidechainTilt)   != nullptr);
}

// ============================================================================
// test_display_mode_parameter_exists
// Verify that displayMode is a choice parameter with exactly 5 options
// (Fast / Slow / SlowDown / Infinite / Off).
// ============================================================================
TEST_CASE("test_display_mode_parameter_exists", "[ParameterState]")
{
    StubProcessor proc;
    auto apvts = makeAPVTS (proc);

    auto* displayModeParam = dynamic_cast<juce::AudioParameterChoice*> (
        apvts.getParameter (ParamID::displayMode));

    REQUIRE (displayModeParam != nullptr);
    REQUIRE (displayModeParam->choices.size() == 5);
    REQUIRE (displayModeParam->choices[0] == "Fast");
    REQUIRE (displayModeParam->choices[4] == "Off");
}
