#include "catch2/catch_amalgamated.hpp"
#include "state/PresetManager.h"

// Minimal AudioProcessor to host APVTS in unit tests.
class ErrorTestProcessor : public juce::AudioProcessor
{
public:
    ErrorTestProcessor()
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

    const juce::String getName() const override { return "ErrorTestProcessor"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
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

// RAII temporary directory for test presets.
struct TempDir
{
    juce::File dir;

    TempDir()
    {
        dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                  .getChildFile("MLIMPresetErrorTest_"
                      + juce::String(juce::Random::getSystemRandom().nextInt64()));
        dir.createDirectory();
    }

    ~TempDir() { dir.deleteRecursively(); }
};

// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_load_nonexistent_preset", "[PresetManagerErrors]")
{
    TempDir tmp;
    ErrorTestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    bool loaded = pm.loadPreset("doesnotexist", proc.apvts);
    REQUIRE(loaded == false);
    // Should not crash; current preset name is unchanged (empty)
    REQUIRE(pm.getCurrentPresetName() == "");
}

// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_load_malformed_xml", "[PresetManagerErrors]")
{
    TempDir tmp;
    ErrorTestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    // Set known parameter values before the attempted load
    auto* gainParam = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("gain"));
    REQUIRE(gainParam != nullptr);
    *gainParam = -12.0f;

    // Write a malformed XML file into the preset directory
    auto badFile = tmp.dir.getChildFile("Broken.xml");
    badFile.replaceWithText("<broken><xml");

    // Load should return false and NOT crash
    bool loaded = pm.loadPreset("Broken", proc.apvts);
    REQUIRE(loaded == false);

    // Parameter state must be unchanged
    REQUIRE(gainParam->get() == Catch::Approx(-12.0f).epsilon(0.01f));
}

// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_save_empty_name", "[PresetManagerErrors]")
{
    TempDir tmp;
    ErrorTestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    // Saving with an empty name should not crash.
    // Behaviour is implementation-defined: either silently ignore or save with
    // an empty filename — both are acceptable as long as there is no crash.
    REQUIRE_NOTHROW(pm.savePreset("", proc.apvts));
}

// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_next_previous_wraps_or_clamps", "[PresetManagerErrors]")
{
    TempDir tmp;
    ErrorTestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    pm.savePreset("A", proc.apvts);
    pm.savePreset("B", proc.apvts);
    pm.savePreset("C", proc.apvts);
    // After saving C, currentPreset == "C" (last saved)

    // Advance past the last preset — should wrap to first, not crash
    pm.nextPreset();  // C -> A (wraps)
    REQUIRE_FALSE(pm.getCurrentPresetName().isEmpty());

    // Advance again several times — must never crash
    for (int i = 0; i < 10; ++i)
        REQUIRE_NOTHROW(pm.nextPreset());
}

// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_previous_at_start", "[PresetManagerErrors]")
{
    TempDir tmp;
    ErrorTestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    pm.savePreset("X", proc.apvts);
    pm.savePreset("Y", proc.apvts);

    // Move to the first preset in sorted order
    // Sorted: X, Y — set current to X by navigating there
    pm.nextPreset();  // Y -> X (wraps)
    REQUIRE(pm.getCurrentPresetName() == "X");

    // Go backwards from the first preset — should wrap to last, not crash
    pm.previousPreset();  // X -> Y (wraps)
    REQUIRE(pm.getCurrentPresetName() == "Y");

    // Call many times — must never crash
    for (int i = 0; i < 10; ++i)
        REQUIRE_NOTHROW(pm.previousPreset());
}

// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_load_preset_missing_parameters", "[PresetManagerErrors]")
{
    TempDir tmp;
    ErrorTestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    // Set both parameters to non-default values
    auto* gainParam = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("gain"));
    auto* ceilParam = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("ceiling"));
    REQUIRE(gainParam != nullptr);
    REQUIRE(ceilParam != nullptr);

    *gainParam = -3.0f;
    *ceilParam = -0.5f;

    // Write a valid XML preset that only contains the "gain" parameter —
    // "ceiling" is missing.  The ValueTree type must match the APVTS ID ("State").
    auto partialFile = tmp.dir.getChildFile("Partial.xml");
    partialFile.replaceWithText(
        R"(<State gain="-3.0"/>)"
    );

    // Loading should not crash.  It may or may not update state, but must
    // return a definitive result (true/false) without throwing or crashing.
    bool loaded = false;
    REQUIRE_NOTHROW(loaded = pm.loadPreset("Partial", proc.apvts));
    // Whether loaded or not, the plugin must remain operable afterwards.
    // Verify the param we can still call getter without crashing.
    REQUIRE_NOTHROW(gainParam->get());
    REQUIRE_NOTHROW(ceilParam->get());
}
