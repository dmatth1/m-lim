#include "catch2/catch_amalgamated.hpp"
#include "state/ABState.h"

// Minimal AudioProcessor used to host an APVTS in unit tests.
class ABTestProcessor : public juce::AudioProcessor
{
public:
    ABTestProcessor()
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
    const juce::String getName() const override { return "ABTestProcessor"; }
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

TEST_CASE("test_capture_and_restore", "[ABState]")
{
    ABTestProcessor proc;
    ABState ab;

    // Set initial values and capture as state A
    setParam(proc.apvts, "gain", 0.0f);
    setParam(proc.apvts, "ceiling", -0.1f);
    ab.captureState(proc.apvts);
    REQUIRE(ab.isA());

    // Change params
    setParam(proc.apvts, "gain", -6.0f);
    setParam(proc.apvts, "ceiling", -3.0f);

    // Restore state A — should recover original values
    ab.restoreState(proc.apvts);

    REQUIRE(getParam(proc.apvts, "gain") == Catch::Approx(0.0f).margin(0.01f));
    REQUIRE(getParam(proc.apvts, "ceiling") == Catch::Approx(-0.1f).margin(0.01f));
}

TEST_CASE("test_toggle_switches", "[ABState]")
{
    ABTestProcessor proc;
    ABState ab;

    // Capture current state as A
    setParam(proc.apvts, "gain", 0.0f);
    ab.captureState(proc.apvts);
    REQUIRE(ab.isA());

    // Toggle to B — A is saved, B is empty so state is left as-is
    ab.toggle(proc.apvts);
    REQUIRE(!ab.isA());

    // Set B params
    setParam(proc.apvts, "gain", -12.0f);
    ab.captureState(proc.apvts);

    // Toggle back to A — should restore gain=0
    ab.toggle(proc.apvts);
    REQUIRE(ab.isA());
    REQUIRE(getParam(proc.apvts, "gain") == Catch::Approx(0.0f).margin(0.01f));

    // Toggle to B again — should restore gain=-12
    ab.toggle(proc.apvts);
    REQUIRE(!ab.isA());
    REQUIRE(getParam(proc.apvts, "gain") == Catch::Approx(-12.0f).margin(0.01f));
}

TEST_CASE("test_copy_a_to_b", "[ABState]")
{
    ABTestProcessor proc;
    ABState ab;

    // Set up state A = {gain=0}: capture while on A
    setParam(proc.apvts, "gain", 0.0f);
    ab.captureState(proc.apvts);
    REQUIRE(ab.isA());

    // Switch to B and set up state B = {gain=-6}
    ab.toggle(proc.apvts);          // saves current (gain=0) to A, switches to B
    REQUIRE(!ab.isA());
    setParam(proc.apvts, "gain", -6.0f);
    ab.captureState(proc.apvts);    // saves B = {gain=-6}

    // Copy A to B — B should now equal A (gain=0)
    ab.copyAtoB();

    // Restore B directly to verify it was overwritten with A's values
    ab.restoreState(proc.apvts);
    REQUIRE(getParam(proc.apvts, "gain") == Catch::Approx(0.0f).margin(0.01f));

    // Also verify A is still untouched
    ab.toggle(proc.apvts);          // saves current (gain=0) to B, switches to A, restores A
    REQUIRE(ab.isA());
    REQUIRE(getParam(proc.apvts, "gain") == Catch::Approx(0.0f).margin(0.01f));
}
