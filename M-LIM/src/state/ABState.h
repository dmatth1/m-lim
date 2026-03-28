#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

/**
 * ABState — A/B parameter comparison snapshot system.
 *
 * Stores two independent snapshots of the APVTS state so users can quickly
 * compare two sets of settings.  One state is "active" at any time.
 */
class ABState
{
public:
    ABState();

    /** Snapshot the current APVTS into whichever slot is active (A or B). */
    void captureState(juce::AudioProcessorValueTreeState& apvts);

    /** Restore the active snapshot into the APVTS. */
    void restoreState(juce::AudioProcessorValueTreeState& apvts);

    /**
     * Toggle the active slot.
     * Before switching, the current APVTS state is captured into the slot that
     * is about to become inactive, then the other slot is restored.
     */
    void toggle(juce::AudioProcessorValueTreeState& apvts);

    /** Copy state A into state B (both become identical; A stays active). */
    void copyAtoB();

    /** Copy state B into state A (both become identical; active slot unchanged). */
    void copyBtoA();

    /** Returns true when state A is the currently active slot. */
    bool isA() const noexcept { return activeIsA; }

    /** Serialize the A/B snapshots and activeIsA flag to an XML element. */
    juce::XmlElement toXml() const;

    /** Restore A/B snapshots and activeIsA flag from an XML element. */
    void fromXml(const juce::XmlElement& xml);

private:
    juce::ValueTree stateA;
    juce::ValueTree stateB;
    bool activeIsA { true };
};
