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

TEST_CASE("test_toggle_to_empty_b_no_crash", "[ABState]")
{
    ABTestProcessor proc;
    ABState ab;

    // Set a known param value, but do NOT capture anything yet
    setParam(proc.apvts, "gain", -3.0f);

    // Toggle before capturing B — should not crash; B is empty so restore is a no-op
    ab.toggle(proc.apvts);   // captures current to A, switches to B, restores B (noop)
    REQUIRE(!ab.isA());

    // Params must still be valid after toggling to an empty slot
    REQUIRE(getParam(proc.apvts, "gain") == Catch::Approx(-3.0f).margin(0.01f));
}

TEST_CASE("test_a_state_isolated_from_b_modifications", "[ABState]")
{
    ABTestProcessor proc;
    ABState ab;

    // Capture A with gain=-3
    setParam(proc.apvts, "gain", -3.0f);
    ab.captureState(proc.apvts);
    REQUIRE(ab.isA());

    // Toggle to B; B is empty so params are unchanged
    ab.toggle(proc.apvts);
    REQUIRE(!ab.isA());

    // Modify params while on B
    setParam(proc.apvts, "gain", -12.0f);

    // Toggle back to A — should restore gain=-3, not -12
    ab.toggle(proc.apvts);
    REQUIRE(ab.isA());
    REQUIRE(getParam(proc.apvts, "gain") == Catch::Approx(-3.0f).margin(0.01f));
}

TEST_CASE("test_recapture_a_overwrites_snapshot", "[ABState]")
{
    ABTestProcessor proc;
    ABState ab;

    // Capture A with gain=0
    setParam(proc.apvts, "gain", 0.0f);
    ab.captureState(proc.apvts);   // A = {gain=0}

    // Change param and re-capture A — overwrites the previous A snapshot
    setParam(proc.apvts, "gain", -6.0f);
    ab.captureState(proc.apvts);   // A = {gain=-6}

    // Toggle to B (empty, restore is noop), trash the live value, toggle back to A
    ab.toggle(proc.apvts);
    setParam(proc.apvts, "gain", -20.0f);
    ab.toggle(proc.apvts);   // saves B, switches to A, restores A = {gain=-6}

    REQUIRE(ab.isA());
    REQUIRE(getParam(proc.apvts, "gain") == Catch::Approx(-6.0f).margin(0.01f));
}

TEST_CASE("test_full_ab_cycle", "[ABState]")
{
    ABTestProcessor proc;
    ABState ab;

    // Establish state A with two distinct param values
    setParam(proc.apvts, "gain", -3.0f);
    setParam(proc.apvts, "ceiling", -1.0f);
    ab.captureState(proc.apvts);
    REQUIRE(ab.isA());

    // Toggle to B
    ab.toggle(proc.apvts);
    REQUIRE(!ab.isA());

    // Modify both params while on B
    setParam(proc.apvts, "gain", -24.0f);
    setParam(proc.apvts, "ceiling", -6.0f);

    // Toggle back to A — all original A values must be restored exactly
    ab.toggle(proc.apvts);
    REQUIRE(ab.isA());
    REQUIRE(getParam(proc.apvts, "gain")    == Catch::Approx(-3.0f).margin(0.01f));
    REQUIRE(getParam(proc.apvts, "ceiling") == Catch::Approx(-1.0f).margin(0.01f));
}

// ---------------------------------------------------------------------------
// test_copy_b_to_a_restores_values
//   Capture A with one set of values, toggle to B, set different values,
//   capture B, then call copyBtoA(). Restore state and verify A's parameter
//   values now match what was in B.
// ---------------------------------------------------------------------------
TEST_CASE("test_copy_b_to_a_restores_values", "[ABState]")
{
    ABTestProcessor proc;
    ABState ab;

    // Establish state A with known values
    setParam(proc.apvts, "gain",    -6.0f);
    setParam(proc.apvts, "ceiling", -1.0f);
    ab.captureState(proc.apvts);   // captures into slot A
    REQUIRE(ab.isA());

    // Toggle to B and set different values
    ab.toggle(proc.apvts);
    REQUIRE(!ab.isA());
    setParam(proc.apvts, "gain",    -12.0f);
    setParam(proc.apvts, "ceiling", -3.0f);
    ab.captureState(proc.apvts);   // captures into slot B

    // Copy B → A
    ab.copyBtoA();

    // Switch back to A and restore — A should now have B's values
    ab.toggle(proc.apvts);         // toggles back to A and restores it
    REQUIRE(ab.isA());
    REQUIRE(getParam(proc.apvts, "gain")    == Catch::Approx(-12.0f).margin(0.01f));
    REQUIRE(getParam(proc.apvts, "ceiling") == Catch::Approx(-3.0f).margin(0.01f));
}

// ---------------------------------------------------------------------------
// test_copy_b_to_a_when_b_not_captured_no_crash
//   Call copyBtoA() without ever calling captureState() in slot B.
//   The method must not throw and must leave parameters with finite values.
// ---------------------------------------------------------------------------
TEST_CASE("test_copy_b_to_a_when_b_not_captured_no_crash", "[ABState]")
{
    ABTestProcessor proc;
    ABState ab;

    // Set known values in slot A only
    setParam(proc.apvts, "gain",    0.0f);
    setParam(proc.apvts, "ceiling", -0.1f);
    ab.captureState(proc.apvts);   // only slot A is captured; B remains empty
    REQUIRE(ab.isA());

    // copyBtoA() with no B captured — must not crash
    REQUIRE_NOTHROW(ab.copyBtoA());

    // After the no-op, restore A and verify parameters are still finite
    ab.restoreState(proc.apvts);
    REQUIRE(std::isfinite(getParam(proc.apvts, "gain")));
    REQUIRE(std::isfinite(getParam(proc.apvts, "ceiling")));
}

// ---------------------------------------------------------------------------
// test_copy_a_to_b_and_b_to_a_roundtrip
//   Symmetric round-trip test: capture A, capture B with different values,
//   copyAtoB() makes B match A, then copyBtoA() makes A match B (which is
//   now a copy of A) — values must be consistent after both copies.
// ---------------------------------------------------------------------------
TEST_CASE("test_copy_a_to_b_and_b_to_a_roundtrip", "[ABState]")
{
    ABTestProcessor proc;
    ABState ab;

    // Establish slot A
    setParam(proc.apvts, "gain",    -3.0f);
    setParam(proc.apvts, "ceiling", -0.5f);
    ab.captureState(proc.apvts);
    REQUIRE(ab.isA());

    // Toggle to B and set different values
    ab.toggle(proc.apvts);
    REQUIRE(!ab.isA());
    setParam(proc.apvts, "gain",    -18.0f);
    setParam(proc.apvts, "ceiling", -6.0f);
    ab.captureState(proc.apvts);

    // Copy A → B: B should now hold A's original values
    ab.copyAtoB();

    // Restore B and check it now has A's values
    ab.restoreState(proc.apvts);
    REQUIRE(getParam(proc.apvts, "gain")    == Catch::Approx(-3.0f).margin(0.01f));
    REQUIRE(getParam(proc.apvts, "ceiling") == Catch::Approx(-0.5f).margin(0.01f));

    // Copy B → A: A should now have whatever B holds (which is a copy of A)
    ab.copyBtoA();

    // Toggle to A and restore — values must still be the original A values
    ab.toggle(proc.apvts);
    REQUIRE(ab.isA());
    ab.restoreState(proc.apvts);
    REQUIRE(getParam(proc.apvts, "gain")    == Catch::Approx(-3.0f).margin(0.01f));
    REQUIRE(getParam(proc.apvts, "ceiling") == Catch::Approx(-0.5f).margin(0.01f));
}
