#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/**
 * PresetManager — saves, loads, and navigates plugin presets.
 *
 * Presets are XML files containing a serialised juce::ValueTree (APVTS state).
 * User presets are stored in the platform app-data directory under
 * "M-LIM/Presets/". The directory can be overridden via setPresetDirectory()
 * (used in tests).
 */
class PresetManager
{
public:
    PresetManager();

    /** Saves the current APVTS state as a named preset. Overwrites if exists. */
    void savePreset(const juce::String& name, juce::AudioProcessorValueTreeState& apvts);

    /** Loads a named preset into the APVTS. Returns false if not found. */
    bool loadPreset(const juce::String& name, juce::AudioProcessorValueTreeState& apvts);

    /** Returns a sorted list of all available preset names (filename without extension). */
    juce::StringArray getPresetNames() const;

    /**
     * Advances to the next preset in the sorted list (wrapping around) AND
     * loads it into the APVTS.  Prefer this over calling nextPreset() +
     * loadPreset() separately — it scans the filesystem only once.
     *
     * @return true if the new preset was found and loaded successfully.
     */
    bool loadNextPreset(juce::AudioProcessorValueTreeState& apvts);

    /**
     * Moves to the previous preset in the sorted list (wrapping around) AND
     * loads it into the APVTS.  Prefer this over calling previousPreset() +
     * loadPreset() separately — it scans the filesystem only once.
     *
     * @return true if the new preset was found and loaded successfully.
     */
    bool loadPreviousPreset(juce::AudioProcessorValueTreeState& apvts);

    /** Returns the name of the currently active preset, or an empty string. */
    juce::String getCurrentPresetName() const;

    /** Override the preset search/save directory (useful for unit tests). */
    void setPresetDirectory(const juce::File& dir);

private:
    juce::File presetDirectory;
    juce::String currentPresetName;

    /**
     * Advances the preset cursor to the next/previous name in the sorted list.
     * Does NOT load the preset into APVTS.  Use loadNextPreset() /
     * loadPreviousPreset() from external code.
     */
    void nextPreset();
    void previousPreset();

    /** Returns the File for a preset name (root directory only for saves). */
    juce::File presetFileForName(const juce::String& name) const;

    /** Searches the directory tree for a preset file by name. */
    juce::File findPresetFile(const juce::String& name) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};
