/**
 * test_parameter_state.cpp — APVTS parameter layout integration tests (Tasks 041, 029)
 *
 * Verifies that all parameters are present in the APVTS layout, that their
 * ranges/defaults match the SPEC, that state serialization round-trips
 * correctly, that A/B switching swaps parameter values, and that undo/redo
 * restores parameter values via juce::UndoManager.
 */
#include "catch2/catch_amalgamated.hpp"
#include "Parameters.h"
#include "state/ABState.h"
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
// Helper: build an APVTS with the full parameter layout (no undo manager)
// ---------------------------------------------------------------------------
static juce::AudioProcessorValueTreeState makeAPVTS (juce::AudioProcessor& owner)
{
    return juce::AudioProcessorValueTreeState (
        owner, nullptr, "Parameters", createParameterLayout());
}

// ============================================================================
// test_all_paramids_accessible
// Every ParamID constant defined in Parameters.h must return a non-null
// RangedAudioParameter from apvts.getParameter().
// ============================================================================
TEST_CASE("test_all_paramids_accessible", "[ParameterState]")
{
    StubProcessor proc;
    auto apvts = makeAPVTS (proc);

    REQUIRE (apvts.getParameter (ParamID::inputGain)             != nullptr);
    REQUIRE (apvts.getParameter (ParamID::outputCeiling)         != nullptr);
    REQUIRE (apvts.getParameter (ParamID::algorithm)             != nullptr);
    REQUIRE (apvts.getParameter (ParamID::lookahead)             != nullptr);
    REQUIRE (apvts.getParameter (ParamID::attack)                != nullptr);
    REQUIRE (apvts.getParameter (ParamID::release)               != nullptr);
    REQUIRE (apvts.getParameter (ParamID::channelLinkTransients) != nullptr);
    REQUIRE (apvts.getParameter (ParamID::channelLinkRelease)    != nullptr);
    REQUIRE (apvts.getParameter (ParamID::truePeakEnabled)       != nullptr);
    REQUIRE (apvts.getParameter (ParamID::oversamplingFactor)    != nullptr);
    REQUIRE (apvts.getParameter (ParamID::dcFilterEnabled)       != nullptr);
    REQUIRE (apvts.getParameter (ParamID::ditherEnabled)         != nullptr);
    REQUIRE (apvts.getParameter (ParamID::ditherBitDepth)        != nullptr);
    REQUIRE (apvts.getParameter (ParamID::ditherNoiseShaping)    != nullptr);
    REQUIRE (apvts.getParameter (ParamID::bypass)                != nullptr);
    REQUIRE (apvts.getParameter (ParamID::unityGainMode)         != nullptr);
    REQUIRE (apvts.getParameter (ParamID::sidechainHPFreq)       != nullptr);
    REQUIRE (apvts.getParameter (ParamID::sidechainLPFreq)       != nullptr);
    REQUIRE (apvts.getParameter (ParamID::sidechainTilt)         != nullptr);
    REQUIRE (apvts.getParameter (ParamID::delta)                 != nullptr);
    REQUIRE (apvts.getParameter (ParamID::displayMode)           != nullptr);
}

// ============================================================================
// test_float_param_ranges
// Float parameters must have the exact min/max/default values declared in
// Parameters.cpp.
// ============================================================================
TEST_CASE("test_float_param_ranges", "[ParameterState]")
{
    StubProcessor proc;
    auto apvts = makeAPVTS (proc);

    auto check = [&] (const juce::String& id,
                      float expectedMin, float expectedMax, float expectedDefault)
    {
        auto* p = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter (id));
        REQUIRE (p != nullptr);
        CHECK (p->range.start == Catch::Approx (expectedMin));
        CHECK (p->range.end   == Catch::Approx (expectedMax));
        CHECK (p->get()       == Catch::Approx (expectedDefault).margin (0.001f));
    };

    check (ParamID::inputGain,             -12.0f,    36.0f,      0.0f);
    check (ParamID::outputCeiling,         -30.0f,     0.0f,     -0.1f);
    check (ParamID::lookahead,               0.0f,     5.0f,      1.0f);
    check (ParamID::attack,                  0.0f,   100.0f,      0.0f);
    check (ParamID::release,                10.0f,  1000.0f,    100.0f);
    check (ParamID::channelLinkTransients,   0.0f,   100.0f,     75.0f);
    check (ParamID::channelLinkRelease,      0.0f,   100.0f,    100.0f);
    check (ParamID::sidechainHPFreq,        20.0f,  2000.0f,     20.0f);
    check (ParamID::sidechainLPFreq,      2000.0f, 20000.0f,  20000.0f);
    check (ParamID::sidechainTilt,          -6.0f,     6.0f,      0.0f);
}

// ============================================================================
// test_bool_params_exist
// All boolean parameters must be accessible and cast correctly to
// AudioParameterBool, and must carry the correct default value.
// ============================================================================
TEST_CASE("test_bool_params_exist", "[ParameterState]")
{
    StubProcessor proc;
    auto apvts = makeAPVTS (proc);

    auto check = [&] (const juce::String& id, bool expectedDefault)
    {
        auto* p = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (id));
        REQUIRE (p != nullptr);
        CHECK (p->get() == expectedDefault);
    };

    check (ParamID::truePeakEnabled,  true);
    check (ParamID::dcFilterEnabled,  false);
    check (ParamID::ditherEnabled,    false);
    check (ParamID::bypass,           false);
    check (ParamID::unityGainMode,    false);
    check (ParamID::delta,            false);
}

// ============================================================================
// test_choice_param_counts
// Choice parameters must have the exact number of options declared in
// Parameters.cpp.
// ============================================================================
TEST_CASE("test_choice_param_counts", "[ParameterState]")
{
    StubProcessor proc;
    auto apvts = makeAPVTS (proc);

    auto check = [&] (const juce::String& id, int expectedCount)
    {
        auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id));
        REQUIRE (p != nullptr);
        CHECK (static_cast<int> (p->choices.size()) == expectedCount);
    };

    check (ParamID::algorithm,          8);  // Transparent/Punchy/Dynamic/Aggressive/Allround/Bus/Safe/Modern
    check (ParamID::oversamplingFactor, 6);  // Off/2x/4x/8x/16x/32x
    check (ParamID::ditherBitDepth,     5);  // 16/18/20/22/24
    check (ParamID::ditherNoiseShaping, 3);  // Basic/Optimized/Weighted
    check (ParamID::displayMode,        5);  // Fast/Slow/SlowDown/Infinite/Off
}

// ============================================================================
// test_param_set_to_extremes
// Setting every parameter to its minimum, maximum, and default must not crash.
// Verifies that the normalisation math handles boundary values correctly.
// ============================================================================
TEST_CASE("test_param_set_to_extremes", "[ParameterState]")
{
    StubProcessor proc;
    auto apvts = makeAPVTS (proc);

    // Float params: set to start, end, then back to default
    const juce::StringArray floatIds {
        ParamID::inputGain, ParamID::outputCeiling, ParamID::lookahead,
        ParamID::attack, ParamID::release, ParamID::channelLinkTransients,
        ParamID::channelLinkRelease, ParamID::sidechainHPFreq,
        ParamID::sidechainLPFreq, ParamID::sidechainTilt
    };
    for (const auto& id : floatIds)
    {
        auto* p = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter (id));
        REQUIRE (p != nullptr);
        *p = p->range.start;
        CHECK (p->get() == Catch::Approx (p->range.start).margin (0.001f));
        *p = p->range.end;
        CHECK (p->get() == Catch::Approx (p->range.end).margin (0.001f));
        *p = p->range.start;  // already set above; just verify no crash
        REQUIRE_NOTHROW (p->get());
    }

    // Bool params: set to true and false
    const juce::StringArray boolIds {
        ParamID::truePeakEnabled, ParamID::dcFilterEnabled,
        ParamID::ditherEnabled, ParamID::bypass,
        ParamID::unityGainMode, ParamID::delta
    };
    for (const auto& id : boolIds)
    {
        auto* p = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (id));
        REQUIRE (p != nullptr);
        *p = true;
        CHECK (p->get() == true);
        *p = false;
        CHECK (p->get() == false);
    }

    // Choice params: set to first and last choice
    const juce::StringArray choiceIds {
        ParamID::algorithm, ParamID::oversamplingFactor,
        ParamID::ditherBitDepth, ParamID::ditherNoiseShaping,
        ParamID::displayMode
    };
    for (const auto& id : choiceIds)
    {
        auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id));
        REQUIRE (p != nullptr);
        *p = 0;
        CHECK (p->getIndex() == 0);
        *p = p->choices.size() - 1;
        CHECK (p->getIndex() == static_cast<int> (p->choices.size()) - 1);
    }
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

// ============================================================================
// test_all_parameters_exist
// Verify that every parameter ID defined in Parameters.h / SPEC exists in the
// APVTS layout returned by createParameterLayout().
// ============================================================================
TEST_CASE("test_all_parameters_exist", "[ParameterState]")
{
    StubProcessor proc;
    auto apvts = makeAPVTS (proc);

    REQUIRE (apvts.getParameter (ParamID::inputGain)              != nullptr);
    REQUIRE (apvts.getParameter (ParamID::outputCeiling)          != nullptr);
    REQUIRE (apvts.getParameter (ParamID::algorithm)              != nullptr);
    REQUIRE (apvts.getParameter (ParamID::lookahead)              != nullptr);
    REQUIRE (apvts.getParameter (ParamID::attack)                 != nullptr);
    REQUIRE (apvts.getParameter (ParamID::release)                != nullptr);
    REQUIRE (apvts.getParameter (ParamID::channelLinkTransients)  != nullptr);
    REQUIRE (apvts.getParameter (ParamID::channelLinkRelease)     != nullptr);
    REQUIRE (apvts.getParameter (ParamID::truePeakEnabled)        != nullptr);
    REQUIRE (apvts.getParameter (ParamID::oversamplingFactor)     != nullptr);
    REQUIRE (apvts.getParameter (ParamID::dcFilterEnabled)        != nullptr);
    REQUIRE (apvts.getParameter (ParamID::ditherEnabled)          != nullptr);
    REQUIRE (apvts.getParameter (ParamID::ditherBitDepth)         != nullptr);
    REQUIRE (apvts.getParameter (ParamID::ditherNoiseShaping)     != nullptr);
    REQUIRE (apvts.getParameter (ParamID::bypass)                 != nullptr);
    REQUIRE (apvts.getParameter (ParamID::unityGainMode)          != nullptr);
    REQUIRE (apvts.getParameter (ParamID::sidechainHPFreq)        != nullptr);
    REQUIRE (apvts.getParameter (ParamID::sidechainLPFreq)        != nullptr);
    REQUIRE (apvts.getParameter (ParamID::sidechainTilt)          != nullptr);
    REQUIRE (apvts.getParameter (ParamID::delta)                  != nullptr);
    REQUIRE (apvts.getParameter (ParamID::displayMode)            != nullptr);
}

// ============================================================================
// test_parameter_ranges
// Verify min/max/default for float parameters and choice sizes match the SPEC.
// ============================================================================
TEST_CASE("test_parameter_ranges", "[ParameterState]")
{
    StubProcessor proc;
    auto apvts = makeAPVTS (proc);

    // --- Float parameters ---
    auto requireFloat = [&] (const juce::String& id,
                              float expectedMin, float expectedMax, float expectedDefault)
    {
        auto* p = dynamic_cast<juce::AudioParameterFloat*> (apvts.getParameter (id));
        REQUIRE (p != nullptr);
        REQUIRE (p->range.start   == Catch::Approx (expectedMin));
        REQUIRE (p->range.end     == Catch::Approx (expectedMax));
        REQUIRE (p->get()         == Catch::Approx (expectedDefault).margin (0.001f));
    };

    requireFloat (ParamID::inputGain,             -12.0f,    36.0f,     0.0f);
    requireFloat (ParamID::outputCeiling,         -30.0f,     0.0f,    -0.1f);
    requireFloat (ParamID::lookahead,               0.0f,     5.0f,     1.0f);
    requireFloat (ParamID::attack,                  0.0f,   100.0f,     0.0f);
    requireFloat (ParamID::release,                10.0f,  1000.0f,   100.0f);
    requireFloat (ParamID::channelLinkTransients,   0.0f,   100.0f,    75.0f);
    requireFloat (ParamID::channelLinkRelease,      0.0f,   100.0f,   100.0f);
    requireFloat (ParamID::sidechainHPFreq,        20.0f,  2000.0f,    20.0f);
    requireFloat (ParamID::sidechainLPFreq,      2000.0f, 20000.0f, 20000.0f);
    requireFloat (ParamID::sidechainTilt,          -6.0f,     6.0f,     0.0f);

    // --- Bool parameters (default values) ---
    auto requireBool = [&] (const juce::String& id, bool expectedDefault)
    {
        auto* p = dynamic_cast<juce::AudioParameterBool*> (apvts.getParameter (id));
        REQUIRE (p != nullptr);
        REQUIRE (p->get() == expectedDefault);
    };

    requireBool (ParamID::truePeakEnabled,  true);
    requireBool (ParamID::dcFilterEnabled,  false);
    requireBool (ParamID::ditherEnabled,    false);
    requireBool (ParamID::bypass,           false);
    requireBool (ParamID::unityGainMode,    false);
    requireBool (ParamID::delta,            false);

    // --- Choice parameters (size checks) ---
    auto requireChoiceSize = [&] (const juce::String& id, int expectedSize)
    {
        auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id));
        REQUIRE (p != nullptr);
        REQUIRE (p->choices.size() == expectedSize);
    };

    requireChoiceSize (ParamID::algorithm,          8);  // Transparent…Modern
    requireChoiceSize (ParamID::oversamplingFactor, 6);  // Off/2x/4x/8x/16x/32x
    requireChoiceSize (ParamID::ditherBitDepth,     5);  // 16/18/20/22/24
    requireChoiceSize (ParamID::ditherNoiseShaping, 3);  // Basic/Optimized/Weighted
    requireChoiceSize (ParamID::displayMode,        5);  // Fast/Slow/SlowDown/Infinite/Off
}

// ============================================================================
// test_state_roundtrip
// Modify several parameters, serialize the APVTS state to a ValueTree, then
// restore it into the same APVTS and verify all values are preserved.
// ============================================================================
TEST_CASE("test_state_roundtrip", "[ParameterState]")
{
    StubProcessor proc;
    auto apvts = makeAPVTS (proc);

    // Set non-default values
    auto* inputGain = dynamic_cast<juce::AudioParameterFloat*> (
        apvts.getParameter (ParamID::inputGain));
    auto* ceiling = dynamic_cast<juce::AudioParameterFloat*> (
        apvts.getParameter (ParamID::outputCeiling));
    auto* truePeak = dynamic_cast<juce::AudioParameterBool*> (
        apvts.getParameter (ParamID::truePeakEnabled));

    REQUIRE (inputGain != nullptr);
    REQUIRE (ceiling   != nullptr);
    REQUIRE (truePeak  != nullptr);

    *inputGain = 6.0f;
    *ceiling   = -1.0f;
    *truePeak  = false;

    // Serialize the current state via copyState() which flushes parameter
    // values to the ValueTree before snapshotting.
    juce::MemoryBlock stateData;
    {
        juce::MemoryOutputStream stream (stateData, false);
        apvts.copyState().writeToStream (stream);
    }
    REQUIRE (stateData.getSize() > 0);

    // Overwrite with different values so we can detect a real restore
    *inputGain = 0.0f;
    *ceiling   = -0.1f;
    *truePeak  = true;

    // Restore
    auto restoredTree = juce::ValueTree::readFromData (
        stateData.getData(), stateData.getSize());
    REQUIRE (restoredTree.isValid());
    apvts.replaceState (restoredTree);

    // Verify all values came back
    REQUIRE (inputGain->get() == Catch::Approx (6.0f).margin (0.01f));
    REQUIRE (ceiling->get()   == Catch::Approx (-1.0f).margin (0.01f));
    REQUIRE (truePeak->get()  == false);
}

// ============================================================================
// test_ab_with_apvts
// Verify that ABState correctly stores and restores parameter values when
// toggling between A and B states using the full parameter layout.
// ============================================================================
TEST_CASE("test_ab_with_apvts", "[ParameterState]")
{
    StubProcessor proc;
    auto apvts = makeAPVTS (proc);
    ABState ab;

    auto* inputGain = dynamic_cast<juce::AudioParameterFloat*> (
        apvts.getParameter (ParamID::inputGain));
    auto* ceiling = dynamic_cast<juce::AudioParameterFloat*> (
        apvts.getParameter (ParamID::outputCeiling));

    REQUIRE (inputGain != nullptr);
    REQUIRE (ceiling   != nullptr);

    // ---- Set up state A ----
    // While on A, set params and capture them.
    *inputGain = 6.0f;
    *ceiling   = -1.0f;
    ab.captureState (apvts);   // stateA = {inputGain=6, ceiling=-1}
    REQUIRE (ab.isA());

    // Toggle to B: toggle() saves current to active (A again, same values),
    // then switches active to B and tries to restore B (empty → unchanged).
    ab.toggle (apvts);
    REQUIRE (!ab.isA());

    // ---- Set up state B ----
    *inputGain = -3.0f;
    *ceiling   = -0.5f;
    ab.captureState (apvts);   // stateB = {inputGain=-3, ceiling=-0.5}

    // Toggle back to A: saves current to B (same values), switches to A, restores A.
    ab.toggle (apvts);
    REQUIRE (ab.isA());
    REQUIRE (inputGain->get() == Catch::Approx (6.0f).margin (0.01f));
    REQUIRE (ceiling->get()   == Catch::Approx (-1.0f).margin (0.01f));

    // Toggle to B again: saves current to A, switches to B, restores B.
    ab.toggle (apvts);
    REQUIRE (!ab.isA());
    REQUIRE (inputGain->get() == Catch::Approx (-3.0f).margin (0.01f));
    REQUIRE (ceiling->get()   == Catch::Approx (-0.5f).margin (0.01f));
}

// ============================================================================
// test_undo_redo_with_apvts
// Verify that changes made through the APVTS Value interface are undoable
// when a juce::UndoManager is attached to the APVTS.
// ============================================================================
TEST_CASE("test_undo_redo_with_apvts", "[ParameterState]")
{
    StubProcessor proc;
    juce::UndoManager undoMgr;
    juce::AudioProcessorValueTreeState apvts (
        proc, &undoMgr, "Parameters", createParameterLayout());

    auto* inputGain = dynamic_cast<juce::AudioParameterFloat*> (
        apvts.getParameter (ParamID::inputGain));
    REQUIRE (inputGain != nullptr);

    // Establish a baseline transaction: set to 3.0
    undoMgr.beginNewTransaction();
    apvts.getParameterAsValue (ParamID::inputGain).setValue (3.0f);
    REQUIRE (inputGain->get() == Catch::Approx (3.0f).margin (0.01f));

    // Start a new transaction and change to -6.0
    undoMgr.beginNewTransaction();
    apvts.getParameterAsValue (ParamID::inputGain).setValue (-6.0f);
    REQUIRE (inputGain->get() == Catch::Approx (-6.0f).margin (0.01f));

    // Undo should restore 3.0
    REQUIRE (undoMgr.canUndo());
    undoMgr.undo();
    REQUIRE (inputGain->get() == Catch::Approx (3.0f).margin (0.01f));

    // Redo should re-apply -6.0
    REQUIRE (undoMgr.canRedo());
    undoMgr.redo();
    REQUIRE (inputGain->get() == Catch::Approx (-6.0f).margin (0.01f));
}
