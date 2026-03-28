#include "state_test_helpers.h"
#include "state/PresetManager.h"

using ErrorTestProcessor = StateTestProcessor;
using TempDir = TempPresetDir;

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
    pm.loadNextPreset(proc.apvts);  // C -> A (wraps)
    REQUIRE_FALSE(pm.getCurrentPresetName().isEmpty());

    // Advance again several times — must never crash
    for (int i = 0; i < 10; ++i)
        REQUIRE_NOTHROW(pm.loadNextPreset(proc.apvts));
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
    pm.loadNextPreset(proc.apvts);  // Y -> X (wraps)
    REQUIRE(pm.getCurrentPresetName() == "X");

    // Go backwards from the first preset — should wrap to last, not crash
    pm.loadPreviousPreset(proc.apvts);  // X -> Y (wraps)
    REQUIRE(pm.getCurrentPresetName() == "Y");

    // Call many times — must never crash
    for (int i = 0; i < 10; ++i)
        REQUIRE_NOTHROW(pm.loadPreviousPreset(proc.apvts));
}

// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_save_unwritable_path_returns_false", "[PresetManagerErrors]")
{
    ErrorTestProcessor proc;
    PresetManager pm;

    // Point the preset directory at a non-existent, non-creatable path.
    // On Linux, /proc/nonexistent is guaranteed to fail on open.
    pm.setPresetDirectory(juce::File("/proc/mlim_unwritable_test_dir"));

    auto* gainParam = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("gain"));
    REQUIRE(gainParam != nullptr);
    *gainParam = -6.0f;

    // savePreset must return false — the directory is not writable.
    bool saved = pm.savePreset("ShouldFail", proc.apvts);
    REQUIRE(saved == false);

    // currentPresetName must NOT be updated to the failed name.
    REQUIRE(pm.getCurrentPresetName() != "ShouldFail");
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

// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_truncated_xml_load_returns_false", "[PresetManagerErrors]")
{
    TempDir tmp;
    ErrorTestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    // First save a valid preset so we have valid XML to truncate from
    pm.savePreset("Valid", proc.apvts);
    auto presetFile = tmp.dir.getChildFile("Valid.xml");
    REQUIRE(presetFile.existsAsFile());

    // Overwrite with just the first 20 bytes (truncated XML)
    auto fullText = presetFile.loadFileAsString();
    REQUIRE(fullText.length() > 20);
    presetFile.replaceWithText(fullText.substring(0, 20));

    // loadPreset must return false, no crash
    bool loaded = pm.loadPreset("Valid", proc.apvts);
    REQUIRE(loaded == false);
}

// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_failed_save_does_not_add_to_index", "[PresetManagerErrors]")
{
    TempDir tmp;
    ErrorTestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    // Save a valid preset first
    pm.savePreset("Good", proc.apvts);

    // Now point preset directory at an unwritable location
    pm.setPresetDirectory(juce::File("/proc/mlim_unwritable_test_dir2"));

    // Attempt to save — should fail
    bool saved = pm.savePreset("Bad", proc.apvts);
    REQUIRE(saved == false);

    // Reset to the temp dir and verify "Bad" is not in the index
    pm.setPresetDirectory(tmp.dir);
    auto names = pm.getPresetNames();
    REQUIRE_FALSE(names.contains("Bad"));
}

// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_whitespace_only_name_no_crash", "[PresetManagerErrors]")
{
    TempDir tmp;
    ErrorTestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    // Saving with whitespace-only name must not crash
    REQUIRE_NOTHROW(pm.savePreset("   ", proc.apvts));
}

// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_unicode_preset_name_roundtrip", "[PresetManagerErrors]")
{
    TempDir tmp;
    ErrorTestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    // Set a known parameter value
    setParam(proc.apvts, "gain", -9.0f);

    // Save with a Unicode name
    juce::String unicodeName = juce::String::fromUTF8("テスト");
    bool saved = pm.savePreset(unicodeName, proc.apvts);
    REQUIRE(saved == true);

    // Change parameter to a different value
    setParam(proc.apvts, "gain", 0.0f);
    REQUIRE(getParam(proc.apvts, "gain") == Catch::Approx(0.0f).epsilon(0.01f));

    // Load the unicode-named preset and verify the parameter is restored
    bool loaded = pm.loadPreset(unicodeName, proc.apvts);
    REQUIRE(loaded == true);
    REQUIRE(getParam(proc.apvts, "gain") == Catch::Approx(-9.0f).epsilon(0.01f));
}

// ─────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_concurrent_save_load_no_crash", "[PresetManagerErrors]")
{
    TempDir tmp;
    ErrorTestProcessor proc1;
    ErrorTestProcessor proc2;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    // Save an initial preset so load has something to find
    pm.savePreset("Concurrent", proc1.apvts);

    std::atomic<bool> running{true};
    std::atomic<bool> crashed{false};

    // Thread 1: repeatedly saves
    std::thread saver([&]() {
        try {
            while (running.load()) {
                pm.savePreset("Concurrent", proc1.apvts);
            }
        } catch (...) {
            crashed.store(true);
        }
    });

    // Thread 2: repeatedly loads
    std::thread loader([&]() {
        try {
            while (running.load()) {
                pm.loadPreset("Concurrent", proc2.apvts);
            }
        } catch (...) {
            crashed.store(true);
        }
    });

    // Let them run for 200ms
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    running.store(false);

    saver.join();
    loader.join();

    REQUIRE(crashed.load() == false);
}
