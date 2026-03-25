#include "catch2/catch_amalgamated.hpp"
#include "state/PresetManager.h"

// Minimal AudioProcessor used to host an APVTS in unit tests.
class TestProcessor : public juce::AudioProcessor
{
public:
    TestProcessor()
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

    // --- Required AudioProcessor overrides ---
    const juce::String getName() const override { return "TestProcessor"; }
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

// Helper: create a temporary directory for test presets and clean it up when done.
struct TempPresetDir
{
    juce::File dir;

    TempPresetDir()
    {
        dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                  .getChildFile("MLIMPresetTest_" + juce::String(juce::Random::getSystemRandom().nextInt64()));
        dir.createDirectory();
    }

    ~TempPresetDir()
    {
        dir.deleteRecursively();
    }
};

// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_save_and_load", "[PresetManager]")
{
    TempPresetDir tmp;
    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    // Set non-default parameter values via AudioParameterFloat operator=
    auto* gainParam    = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("gain"));
    auto* ceilParam    = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("ceiling"));
    REQUIRE(gainParam    != nullptr);
    REQUIRE(ceilParam    != nullptr);

    *gainParam = -6.0f;
    *ceilParam = -1.0f;

    // Save preset
    pm.savePreset("MyPreset", proc.apvts);
    REQUIRE(pm.getCurrentPresetName() == "MyPreset");

    // Mutate parameters back to defaults
    *gainParam = 0.0f;
    *ceilParam = -0.1f;

    // Load should restore saved values
    bool loaded = pm.loadPreset("MyPreset", proc.apvts);
    REQUIRE(loaded == true);
    REQUIRE(pm.getCurrentPresetName() == "MyPreset");

    REQUIRE(gainParam->get()  == Catch::Approx(-6.0f).epsilon(0.01f));
    REQUIRE(ceilParam->get()  == Catch::Approx(-1.0f).epsilon(0.01f));
}

// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_get_preset_names", "[PresetManager]")
{
    TempPresetDir tmp;
    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    // No presets yet
    REQUIRE(pm.getPresetNames().size() == 0);

    pm.savePreset("Alpha", proc.apvts);
    pm.savePreset("Beta",  proc.apvts);
    pm.savePreset("Gamma", proc.apvts);

    auto names = pm.getPresetNames();
    REQUIRE(names.size() == 3);
    // Names should be sorted
    REQUIRE(names[0] == "Alpha");
    REQUIRE(names[1] == "Beta");
    REQUIRE(names[2] == "Gamma");
}

// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_next_previous", "[PresetManager]")
{
    TempPresetDir tmp;
    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    pm.savePreset("A", proc.apvts);
    pm.savePreset("B", proc.apvts);
    pm.savePreset("C", proc.apvts);

    // After saving "C" the current preset should be "C"
    REQUIRE(pm.getCurrentPresetName() == "C");

    // nextPreset wraps: C -> A (sorted order: A, B, C)
    pm.nextPreset();
    REQUIRE(pm.getCurrentPresetName() == "A");

    pm.nextPreset();
    REQUIRE(pm.getCurrentPresetName() == "B");

    pm.nextPreset();
    REQUIRE(pm.getCurrentPresetName() == "C");

    // previousPreset wraps back
    pm.previousPreset();
    REQUIRE(pm.getCurrentPresetName() == "B");

    pm.previousPreset();
    REQUIRE(pm.getCurrentPresetName() == "A");

    // Wrap around going backwards: A -> C
    pm.previousPreset();
    REQUIRE(pm.getCurrentPresetName() == "C");
}

// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_factory_presets_exist", "[PresetManager]")
{
    // Point PresetManager at the source-tree presets directory which contains
    // the shipped factory presets.  The path is injected at compile time.
#ifndef MLIM_PRESETS_DIR
    SKIP("MLIM_PRESETS_DIR not defined — skipping factory preset test");
#else
    juce::File presetsDir(MLIM_PRESETS_DIR);
    REQUIRE(presetsDir.isDirectory());

    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(presetsDir);

    auto names = pm.getPresetNames();
    REQUIRE(names.contains("Default"));
    REQUIRE(names.contains("Transparent_Master"));
    REQUIRE(names.contains("Loud_Master"));
    REQUIRE(names.contains("Bus_Glue"));
    REQUIRE(names.contains("Drum_Bus"));
    REQUIRE(names.contains("EBU_R128"));
    REQUIRE(names.contains("Streaming"));

    // Verify at least one can be loaded
    bool loaded = pm.loadPreset("Default", proc.apvts);
    REQUIRE(loaded == true);
    REQUIRE(pm.getCurrentPresetName() == "Default");
#endif
}
