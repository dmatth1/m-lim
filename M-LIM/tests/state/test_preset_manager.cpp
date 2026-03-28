#include "state_test_helpers.h"
#include "state/PresetManager.h"

using TestProcessor = StateTestProcessor;

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

    // loadNextPreset wraps: C -> A (sorted order: A, B, C)
    REQUIRE(pm.loadNextPreset(proc.apvts) == true);
    REQUIRE(pm.getCurrentPresetName() == "A");

    REQUIRE(pm.loadNextPreset(proc.apvts) == true);
    REQUIRE(pm.getCurrentPresetName() == "B");

    REQUIRE(pm.loadNextPreset(proc.apvts) == true);
    REQUIRE(pm.getCurrentPresetName() == "C");

    // loadPreviousPreset wraps back
    REQUIRE(pm.loadPreviousPreset(proc.apvts) == true);
    REQUIRE(pm.getCurrentPresetName() == "B");

    REQUIRE(pm.loadPreviousPreset(proc.apvts) == true);
    REQUIRE(pm.getCurrentPresetName() == "A");

    // Wrap around going backwards: A -> C
    REQUIRE(pm.loadPreviousPreset(proc.apvts) == true);
    REQUIRE(pm.getCurrentPresetName() == "C");
}

// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_save_overwrite_no_duplicate", "[PresetManager]")
{
    TempPresetDir tmp;
    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    auto* gainParam = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("gain"));
    REQUIRE(gainParam != nullptr);

    // First save with gain = -6
    *gainParam = -6.0f;
    pm.savePreset("MyPreset", proc.apvts);

    // Second save with gain = -12 (same name, different value)
    *gainParam = -12.0f;
    pm.savePreset("MyPreset", proc.apvts);

    // Must have exactly 1 entry — not 2
    auto names = pm.getPresetNames();
    REQUIRE(names.size() == 1);
    REQUIRE(names[0] == "MyPreset");

    // Load should restore the second save (gain = -12)
    *gainParam = 0.0f;
    bool loaded = pm.loadPreset("MyPreset", proc.apvts);
    REQUIRE(loaded == true);
    REQUIRE(gainParam->get() == Catch::Approx(-12.0f).epsilon(0.01f));
}

// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_special_chars_no_crash", "[PresetManager]")
{
    TempPresetDir tmp;
    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    // Names with path-separator / filesystem-illegal characters must not crash.
    // The implementation may silently fail to write (acceptable) but must not throw.
    REQUIRE_NOTHROW(pm.savePreset("Test/Preset:Name",    proc.apvts));
    REQUIRE_NOTHROW(pm.savePreset("Bad\\Name",           proc.apvts));
    REQUIRE_NOTHROW(pm.savePreset("Qu?ery*File<>|",      proc.apvts));
    REQUIRE_NOTHROW(pm.savePreset("Quote\"Name",         proc.apvts));
}

// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_long_name_no_crash", "[PresetManager]")
{
    TempPresetDir tmp;
    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    // 300-character name exceeds the 255-byte Linux filename limit.
    // Must not crash — may silently fail to save.
    juce::String longName(juce::String::repeatedString("A", 300));
    REQUIRE(longName.length() == 300);
    REQUIRE_NOTHROW(pm.savePreset(longName, proc.apvts));
}

// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_save_load_parameter_fidelity", "[PresetManager]")
{
    TempPresetDir tmp;
    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    auto* gainParam = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("gain"));
    auto* ceilParam = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("ceiling"));
    REQUIRE(gainParam != nullptr);
    REQUIRE(ceilParam != nullptr);

    // Set non-default, non-round values to catch floating-point serialisation issues
    *gainParam = -18.5f;
    *ceilParam = -2.7f;

    pm.savePreset("FidelityTest", proc.apvts);

    // Reset to defaults before loading
    *gainParam = 0.0f;
    *ceilParam = -0.1f;

    bool loaded = pm.loadPreset("FidelityTest", proc.apvts);
    REQUIRE(loaded == true);

    // Both parameter values must be faithfully restored
    REQUIRE(gainParam->get() == Catch::Approx(-18.5f).epsilon(0.01f));
    REQUIRE(ceilParam->get() == Catch::Approx(-2.7f).epsilon(0.01f));
}

// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_empty_preset_list_navigate_no_crash", "[PresetManager]")
{
    TempPresetDir tmp;
    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    // No presets have been saved — navigation must not crash and returns false
    REQUIRE(pm.getPresetNames().isEmpty());
    REQUIRE(pm.loadNextPreset(proc.apvts) == false);
    REQUIRE(pm.loadPreviousPreset(proc.apvts) == false);
    REQUIRE(pm.loadNextPreset(proc.apvts) == false);

    // Current preset name remains empty (or at least consistent)
    REQUIRE_NOTHROW(pm.getCurrentPresetName());
}

// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_load_next_advances_and_loads", "[PresetManager]")
{
    TempPresetDir tmp;
    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    auto* gainParam = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("gain"));
    auto* ceilParam = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("ceiling"));
    REQUIRE(gainParam != nullptr);
    REQUIRE(ceilParam != nullptr);

    // Save preset "Alpha" with gain = -6
    *gainParam = -6.0f;
    *ceilParam = -1.0f;
    pm.savePreset("Alpha", proc.apvts);

    // Save preset "Beta" with gain = -12
    *gainParam = -12.0f;
    *ceilParam = -2.0f;
    pm.savePreset("Beta", proc.apvts);

    // After saving "Beta", currentPresetName == "Beta"
    REQUIRE(pm.getCurrentPresetName() == "Beta");

    // loadNextPreset should: advance Beta -> Alpha (wrap) AND load Alpha's values
    bool ok = pm.loadNextPreset(proc.apvts);
    REQUIRE(ok == true);

    // Name must have advanced
    REQUIRE(pm.getCurrentPresetName() == "Alpha");

    // APVTS state must reflect Alpha's saved values — not Beta's
    REQUIRE(gainParam->get() == Catch::Approx(-6.0f).epsilon(0.01f));
    REQUIRE(ceilParam->get() == Catch::Approx(-1.0f).epsilon(0.01f));
}

// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_load_previous_advances_and_loads", "[PresetManager]")
{
    TempPresetDir tmp;
    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    auto* gainParam = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("gain"));
    REQUIRE(gainParam != nullptr);

    // Save two presets: "X" (gain = -3) and "Y" (gain = -9)
    *gainParam = -3.0f;
    pm.savePreset("X", proc.apvts);
    *gainParam = -9.0f;
    pm.savePreset("Y", proc.apvts);

    // Navigate to "X" first (loadNext wraps Y -> X)
    pm.loadNextPreset(proc.apvts);
    REQUIRE(pm.getCurrentPresetName() == "X");

    // loadPreviousPreset: X -> Y (wrap) and should load Y's gain = -9
    bool ok = pm.loadPreviousPreset(proc.apvts);
    REQUIRE(ok == true);
    REQUIRE(pm.getCurrentPresetName() == "Y");
    REQUIRE(gainParam->get() == Catch::Approx(-9.0f).epsilon(0.01f));
}

// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_load_next_twice_advances_from_last_loaded", "[PresetManager]")
{
    TempPresetDir tmp;
    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    auto* gainParam = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("gain"));
    REQUIRE(gainParam != nullptr);

    // Save three presets with distinct gain values (sorted order: A, B, C)
    *gainParam = -3.0f;
    pm.savePreset("A", proc.apvts);
    *gainParam = -6.0f;
    pm.savePreset("B", proc.apvts);
    *gainParam = -9.0f;
    pm.savePreset("C", proc.apvts);

    // After saving "C" the cursor is at "C"
    REQUIRE(pm.getCurrentPresetName() == "C");

    // First loadNextPreset: C -> A (wrap)
    bool ok1 = pm.loadNextPreset(proc.apvts);
    REQUIRE(ok1 == true);
    REQUIRE(pm.getCurrentPresetName() == "A");
    REQUIRE(gainParam->get() == Catch::Approx(-3.0f).epsilon(0.01f));

    // Second loadNextPreset: A -> B (continues from where the first call left off)
    bool ok2 = pm.loadNextPreset(proc.apvts);
    REQUIRE(ok2 == true);
    REQUIRE(pm.getCurrentPresetName() == "B");
    REQUIRE(gainParam->get() == Catch::Approx(-6.0f).epsilon(0.01f));
}

// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_save_overwrites_existing", "[PresetManager]")
{
    TempPresetDir tmp;
    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    auto* gainParam = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("gain"));
    REQUIRE(gainParam != nullptr);

    // First save: Alpha with gain = -6
    *gainParam = -6.0f;
    pm.savePreset("Alpha", proc.apvts);
    REQUIRE(pm.getCurrentPresetName() == "Alpha");

    // Second save: Alpha with gain = -12 (overwrite)
    *gainParam = -12.0f;
    pm.savePreset("Alpha", proc.apvts);
    REQUIRE(pm.getCurrentPresetName() == "Alpha");

    // Exactly one preset in the list
    auto names = pm.getPresetNames();
    REQUIRE(names.size() == 1);
    REQUIRE(names[0] == "Alpha");

    // File count in directory must be exactly 1
    auto files = tmp.dir.findChildFiles(juce::File::findFiles, false, "*.xml");
    REQUIRE(files.size() == 1);

    // Load should restore the second save (gain = -12, not -6)
    *gainParam = 0.0f;
    bool loaded = pm.loadPreset("Alpha", proc.apvts);
    REQUIRE(loaded == true);
    REQUIRE(pm.getCurrentPresetName() == "Alpha");
    REQUIRE(gainParam->get() == Catch::Approx(-12.0f).epsilon(0.01f));
}

// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_overwrite_does_not_duplicate_index", "[PresetManager]")
{
    TempPresetDir tmp;
    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    auto* gainParam = dynamic_cast<juce::AudioParameterFloat*>(proc.apvts.getParameter("gain"));
    REQUIRE(gainParam != nullptr);

    // Save "Beta" three times with different values
    *gainParam = -3.0f;
    pm.savePreset("Beta", proc.apvts);
    *gainParam = -6.0f;
    pm.savePreset("Beta", proc.apvts);
    *gainParam = -9.0f;
    pm.savePreset("Beta", proc.apvts);

    // Preset list must contain "Beta" exactly once
    auto names = pm.getPresetNames();
    REQUIRE(names.size() == 1);
    REQUIRE(names[0] == "Beta");

    // File count in directory must be exactly 1
    auto files = tmp.dir.findChildFiles(juce::File::findFiles, false, "*.xml");
    REQUIRE(files.size() == 1);

    // loadNextPreset from "Beta" (only preset) must wrap back to "Beta" — no duplicates
    *gainParam = 0.0f;
    bool ok = pm.loadNextPreset(proc.apvts);
    REQUIRE(ok == true);
    REQUIRE(pm.getCurrentPresetName() == "Beta");
    // Must have loaded the last save (gain = -9)
    REQUIRE(gainParam->get() == Catch::Approx(-9.0f).epsilon(0.01f));
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

// ────────────────────────────────────────────────────────────────────────────
// test_dotdot_traversal_save_no_escape
//
// savePreset("../evil", ...) must not write a file outside the preset dir.
// Either it sanitises the name and writes safely inside the dir, or it
// returns false / writes nothing.  Either way the parent directory must NOT
// contain "evil.xml".
// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_dotdot_traversal_save_no_escape", "[PresetManager]")
{
    TempPresetDir tmp;
    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    // Attempt to write a file outside the preset directory via ../
    pm.savePreset("../evil", proc.apvts);

    // The file must NOT have been created in the parent directory
    auto escaped = tmp.dir.getParentDirectory().getChildFile("evil.xml");
    // Clean up in case it was accidentally created, then fail the test
    bool wasCreated = escaped.existsAsFile();
    escaped.deleteFile();
    REQUIRE_FALSE(wasCreated);
}

// ────────────────────────────────────────────────────────────────────────────
// test_dotdot_traversal_load_returns_false
//
// loadPreset("../evil", ...) for a non-existent file outside the preset dir
// must return false without reading anything.
// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_dotdot_traversal_load_returns_false", "[PresetManager]")
{
    TempPresetDir tmp;
    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    // Ensure no "evil.xml" exists outside the preset dir
    auto outsideFile = tmp.dir.getParentDirectory().getChildFile("evil.xml");
    outsideFile.deleteFile();

    bool result = pm.loadPreset("../evil", proc.apvts);
    REQUIRE(result == false);
}

// ────────────────────────────────────────────────────────────────────────────
// test_nested_dotdot_traversal_no_escape
//
// savePreset("../../deep/escape", ...) must not write outside the preset dir
// regardless of nesting depth.
// ────────────────────────────────────────────────────────────────────────────
TEST_CASE("test_nested_dotdot_traversal_no_escape", "[PresetManager]")
{
    TempPresetDir tmp;
    TestProcessor proc;
    PresetManager pm;
    pm.setPresetDirectory(tmp.dir);

    pm.savePreset("../../deep/escape", proc.apvts);

    // Walk up two levels and check for "deep/escape.xml"
    auto twoUp = tmp.dir.getParentDirectory().getParentDirectory();
    auto escaped = twoUp.getChildFile("deep").getChildFile("escape.xml");
    bool wasCreated = escaped.existsAsFile();
    escaped.deleteFile();
    REQUIRE_FALSE(wasCreated);
}
