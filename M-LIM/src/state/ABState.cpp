#include "ABState.h"

ABState::ABState()
    : stateA (juce::ValueTree ("ABState")),
      stateB (juce::ValueTree ("ABState"))
{
    // Both slots start as valid (but empty) ValueTrees so that isValid() == true
    // even before the first captureState() call.
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
    jassert (stateA.isValid());
    jassert (stateB.isValid());
    const juce::ValueTree& src = activeIsA ? stateA : stateB;
    if (src.isValid() && src.getNumChildren() > 0)
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

juce::XmlElement ABState::toXml() const
{
    juce::XmlElement xml ("ABState");
    xml.setAttribute ("activeIsA", activeIsA ? 1 : 0);

    if (stateA.isValid())
    {
        auto stateAXml = stateA.createXml();
        if (stateAXml != nullptr)
        {
            auto* wrapper = xml.createNewChildElement ("StateA");
            wrapper->addChildElement (stateAXml.release());
        }
    }

    if (stateB.isValid())
    {
        auto stateBXml = stateB.createXml();
        if (stateBXml != nullptr)
        {
            auto* wrapper = xml.createNewChildElement ("StateB");
            wrapper->addChildElement (stateBXml.release());
        }
    }

    return xml;
}

void ABState::fromXml (const juce::XmlElement& xml)
{
    activeIsA = xml.getIntAttribute ("activeIsA", 1) != 0;

    if (auto* stateAXml = xml.getChildByName ("StateA"))
    {
        if (auto* inner = stateAXml->getFirstChildElement())
            stateA = juce::ValueTree::fromXml (*inner);
    }
    else
    {
        stateA = juce::ValueTree ("ABState");
    }

    if (auto* stateBXml = xml.getChildByName ("StateB"))
    {
        if (auto* inner = stateBXml->getFirstChildElement())
            stateB = juce::ValueTree::fromXml (*inner);
    }
    else
    {
        stateB = juce::ValueTree ("ABState");
    }
}
