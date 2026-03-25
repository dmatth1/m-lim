#include "ABState.h"

ABState::ABState()
{
    // Both slots start empty; they will be populated on the first captureState call.
}

void ABState::captureState(juce::AudioProcessorValueTreeState& apvts)
{
    juce::ValueTree snapshot = apvts.copyState();
    if (activeIsA)
        stateA = snapshot;
    else
        stateB = snapshot;
}

void ABState::restoreState(juce::AudioProcessorValueTreeState& apvts)
{
    const juce::ValueTree& src = activeIsA ? stateA : stateB;
    if (src.isValid())
        apvts.replaceState(src.createCopy());
}

void ABState::toggle(juce::AudioProcessorValueTreeState& apvts)
{
    // Capture current state into the active slot before switching.
    captureState(apvts);

    // Switch the active slot.
    activeIsA = !activeIsA;

    // Restore from the newly active slot.
    restoreState(apvts);
}

void ABState::copyAtoB()
{
    if (stateA.isValid())
        stateB = stateA.createCopy();
}

void ABState::copyBtoA()
{
    if (stateB.isValid())
        stateA = stateB.createCopy();
}
