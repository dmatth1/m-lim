#include "state/PresetManager.h"

PresetManager::PresetManager()
{
    presetDirectory = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("M-LIM/Presets");
    presetDirectory.createDirectory();
}

void PresetManager::setPresetDirectory(const juce::File& dir)
{
    presetDirectory = dir;
    presetDirectory.createDirectory();
}

bool PresetManager::savePreset(const juce::String& name,
                               juce::AudioProcessorValueTreeState& apvts)
{
    auto state = apvts.copyState();
    auto xml = state.createXml();
    if (xml == nullptr)
        return false;

    auto file = presetFileForName(name);
    juce::FileOutputStream stream(file);
    if (!stream.openedOk())
        return false;

    stream.setPosition(0);
    stream.truncate();
    xml->writeTo(stream);
    currentPresetName = name;
    return true;
}

bool PresetManager::loadPreset(const juce::String& name,
                               juce::AudioProcessorValueTreeState& apvts)
{
    auto file = findPresetFile(name);
    if (!file.existsAsFile())
        return false;

    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr)
        return false;

    auto state = juce::ValueTree::fromXml(*xml);
    if (state.isValid())
    {
        apvts.replaceState(state);
        currentPresetName = name;
        return true;
    }
    return false;
}

juce::StringArray PresetManager::getPresetNames() const
{
    juce::StringArray names;

    auto files = presetDirectory.findChildFiles(juce::File::findFiles, true, "*.xml");
    files.sort();

    for (const auto& file : files)
        names.addIfNotAlreadyThere(file.getFileNameWithoutExtension());

    names.sort(false);
    return names;
}

juce::String PresetManager::stepPreset(int direction)
{
    auto names = getPresetNames();
    if (names.isEmpty())
        return {};

    int index = names.indexOf(currentPresetName);
    if (index < 0)
        index = 0;
    else
        index = (index + direction + names.size()) % names.size();

    currentPresetName = names[index];
    return currentPresetName;
}

bool PresetManager::loadNextPreset(juce::AudioProcessorValueTreeState& apvts)
{
    auto name = stepPreset(+1);
    if (name.isEmpty())
        return false;
    return loadPreset(name, apvts);
}

bool PresetManager::loadPreviousPreset(juce::AudioProcessorValueTreeState& apvts)
{
    auto name = stepPreset(-1);
    if (name.isEmpty())
        return false;
    return loadPreset(name, apvts);
}

juce::String PresetManager::getCurrentPresetName() const
{
    return currentPresetName;
}

void PresetManager::setCurrentPresetName(const juce::String& name)
{
    currentPresetName = name;
}

juce::File PresetManager::presetFileForName(const juce::String& name) const
{
    auto candidate = presetDirectory.getChildFile(name + ".xml");
    // Reject paths that resolve outside the preset directory (e.g. "../evil").
    if (!candidate.isAChildOf(presetDirectory) && candidate != presetDirectory)
        return {};
    return candidate;
}

juce::File PresetManager::findPresetFile(const juce::String& name) const
{
    // First check the root
    auto root = presetFileForName(name);
    if (root.existsAsFile())
        return root;

    // Search recursively for a matching filename
    auto files = presetDirectory.findChildFiles(juce::File::findFiles, true,
                                                name + ".xml");
    if (!files.isEmpty())
        return files.getFirst();

    return {};
}
