#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

MLIMAudioProcessor::MLIMAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undoManager, "Parameters", createParameterLayout())
{
    initParameterPointers();

    // Listen for ALL parameter changes — sets the dirty flag so processBlock
    // knows to call pushAllParametersToEngine(). Oversampling/lookahead IDs also
    // get their special handling inside parameterChanged().
    for (auto* param : apvts.processor.getParameters())
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(param))
            apvts.addParameterListener (p->getParameterID(), this);
    }
}

MLIMAudioProcessor::~MLIMAudioProcessor()
{
    for (auto* param : apvts.processor.getParameters())
    {
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*>(param))
            apvts.removeParameterListener (p->getParameterID(), this);
    }
}

// ---------------------------------------------------------------------------
// AudioProcessor overrides
// ---------------------------------------------------------------------------

const juce::String MLIMAudioProcessor::getName() const { return JucePlugin_Name; }

bool MLIMAudioProcessor::acceptsMidi() const  { return false; }
bool MLIMAudioProcessor::producesMidi() const { return false; }
bool MLIMAudioProcessor::isMidiEffect() const { return false; }
double MLIMAudioProcessor::getTailLengthSeconds() const
{
    const double sr = getSampleRate();
    if (sr > 0.0)
        return static_cast<double>(getLatencySamples()) / sr;
    // Fallback before prepareToPlay (sr == 0): use lookahead only
    if (pLookahead != nullptr)
        return static_cast<double>(pLookahead->load()) * 0.001;
    return 0.0;
}

int MLIMAudioProcessor::getNumPrograms()                              { return 1; }
int MLIMAudioProcessor::getCurrentProgram()                           { return 0; }
void MLIMAudioProcessor::setCurrentProgram (int)                      {}
const juce::String MLIMAudioProcessor::getProgramName (int)           { return {}; }
void MLIMAudioProcessor::changeProgramName (int, const juce::String&) {}

// ---------------------------------------------------------------------------
// prepareToPlay
// ---------------------------------------------------------------------------

void MLIMAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const int numChannels = std::max (
        getTotalNumInputChannels(), getTotalNumOutputChannels());

    // Apply any pending oversampling factor change (memory allocation happens here,
    // not on the audio thread)
    if (mOversamplingChangePending.exchange (false))
    {
        const int factor = mPendingOversamplingFactor.load();
        limiterEngine.setOversamplingFactor (factor);
    }

    limiterEngine.prepare (sampleRate, samplesPerBlock, numChannels);
    loudnessMeter.prepare (sampleRate, numChannels);

    // Push all current parameter values to the engine
    pushAllParametersToEngine();

    // Report latency to the host
    updateLatency();
}

void MLIMAudioProcessor::releaseResources()
{
    MeterData md;
    while (mProcessorMeterFIFO.pop(md)) {}
}

void MLIMAudioProcessor::reset()
{
    limiterEngine.reset();
    loudnessMeter.resetIntegrated();
}

void MLIMAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer& midiMessages)
{
    // The host calls this when its own bypass is engaged, but M-LIM reports
    // non-zero latency (lookahead + oversampler). We must maintain that latency
    // so delay-compensated tracks stay aligned. Run processBlock normally —
    // it respects the engine bypass flag for gain reduction while still applying
    // the lookahead delay path.
    processBlock (buffer, midiMessages);
}

bool MLIMAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

// ---------------------------------------------------------------------------
// processBlock — audio thread
// ---------------------------------------------------------------------------

void MLIMAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    // Push parameters to the engine only when something has changed (dirty flag set
    // by parameterChanged on the message thread, or by prepareToPlay). This avoids
    // ~20 atomic loads + function calls per buffer in the common steady-state case.
    // NOTE: oversamplingFactor changes are deferred — do NOT call setOversamplingFactor
    // here (it allocates memory). Changes are handled via parameterChanged + prepareToPlay.
    if (mParametersDirty.exchange (false))
        pushAllParametersToEngine();

    limiterEngine.process (buffer);

    // Measure output loudness (post-limiting signal)
    loudnessMeter.processBlock (buffer);

    // Drain engine FIFO, augment with LUFS values, push to processor FIFO
    MeterData md;
    while (limiterEngine.getMeterFIFO().pop (md))
    {
        md.momentaryLUFS  = loudnessMeter.getMomentaryLUFS();
        md.shortTermLUFS  = loudnessMeter.getShortTermLUFS();
        md.integratedLUFS = loudnessMeter.getIntegratedLUFS();
        md.loudnessRange  = loudnessMeter.getLoudnessRange();
        mProcessorMeterFIFO.push (md);
    }
}

// ---------------------------------------------------------------------------
// Editor
// ---------------------------------------------------------------------------

bool MLIMAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* MLIMAudioProcessor::createEditor()
{
    return new MLIMAudioProcessorEditor (*this);
}

// ---------------------------------------------------------------------------
// State save / load
// ---------------------------------------------------------------------------

void MLIMAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto root = std::make_unique<juce::XmlElement> ("MLIMState");

    // APVTS parameters
    auto apvtsState = apvts.copyState();
    auto apvtsXml = apvtsState.createXml();
    if (apvtsXml != nullptr)
        root->addChildElement (apvtsXml.release());

    // A/B state
    root->addChildElement (new juce::XmlElement (abState.toXml()));

    copyXmlToBinary (*root, destData);
}

void MLIMAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState == nullptr)
        return;

    if (xmlState->hasTagName ("MLIMState"))
    {
        // New format with MLIMState wrapper
        if (auto* apvtsXml = xmlState->getChildByName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*apvtsXml));

        if (auto* abXml = xmlState->getChildByName ("ABState"))
            abState.fromXml (*abXml);

        updateLatency();
    }
    else if (xmlState->hasTagName (apvts.state.getType()))
    {
        // Old format — APVTS root tag only, no A/B state
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
        updateLatency();
    }
}

// ---------------------------------------------------------------------------
// AsyncUpdater — apply deferred oversampling changes on the message thread
// ---------------------------------------------------------------------------

void MLIMAudioProcessor::handleAsyncUpdate()
{
    if (!mOversamplingChangePending.exchange (false))
        return;

    const int factor = mPendingOversamplingFactor.load();

    // Suspend audio processing while we reallocate (message thread, safe to alloc)
    suspendProcessing (true);

    limiterEngine.setOversamplingFactor (factor);

    // Re-prepare the engine with the current audio session settings
    const double sr        = getSampleRate();
    const int    blockSize = getBlockSize();
    if (sr > 0.0 && blockSize > 0)
    {
        const int numCh = std::max (getTotalNumInputChannels(), getTotalNumOutputChannels());
        limiterEngine.prepare (sr, blockSize, numCh);
        loudnessMeter.prepare (sr, numCh);
        pushAllParametersToEngine();
    }

    suspendProcessing (false);
    updateLatency();
}

// ---------------------------------------------------------------------------
// APVTS Listener — called on message thread when parameters change
// ---------------------------------------------------------------------------

void MLIMAudioProcessor::parameterChanged (const juce::String& paramID, float newValue)
{
    // Mark parameters dirty so processBlock will push them on the next audio callback.
    mParametersDirty.store (true);

    if (paramID == ParamID::oversamplingFactor)
    {
        mPendingOversamplingFactor.store (static_cast<int> (newValue));
        mOversamplingChangePending.store (true);
        // Schedule rebuild on the message thread via AsyncUpdater.
        // handleAsyncUpdate() will suspendProcessing, rebuild, and update latency.
        triggerAsyncUpdate();
    }
    else if (paramID == ParamID::lookahead)
    {
        // Lookahead change updates latency; apply immediately (engine setter is
        // atomic-safe and the latency report must be updated on the message thread)
        limiterEngine.setLookahead (newValue);
        updateLatency();
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void MLIMAudioProcessor::initParameterPointers()
{
    pInputGain             = apvts.getRawParameterValue (ParamID::inputGain);
    jassert (pInputGain != nullptr);
    pOutputCeiling         = apvts.getRawParameterValue (ParamID::outputCeiling);
    jassert (pOutputCeiling != nullptr);
    pAlgorithm             = apvts.getRawParameterValue (ParamID::algorithm);
    jassert (pAlgorithm != nullptr);
    pLookahead             = apvts.getRawParameterValue (ParamID::lookahead);
    jassert (pLookahead != nullptr);
    pAttack                = apvts.getRawParameterValue (ParamID::attack);
    jassert (pAttack != nullptr);
    pRelease               = apvts.getRawParameterValue (ParamID::release);
    jassert (pRelease != nullptr);
    pChannelLinkTransients = apvts.getRawParameterValue (ParamID::channelLinkTransients);
    jassert (pChannelLinkTransients != nullptr);
    pChannelLinkRelease    = apvts.getRawParameterValue (ParamID::channelLinkRelease);
    jassert (pChannelLinkRelease != nullptr);
    pTruePeakEnabled       = apvts.getRawParameterValue (ParamID::truePeakEnabled);
    jassert (pTruePeakEnabled != nullptr);
    pOversamplingFactor    = apvts.getRawParameterValue (ParamID::oversamplingFactor);
    jassert (pOversamplingFactor != nullptr);
    pDCFilterEnabled       = apvts.getRawParameterValue (ParamID::dcFilterEnabled);
    jassert (pDCFilterEnabled != nullptr);
    pDitherEnabled         = apvts.getRawParameterValue (ParamID::ditherEnabled);
    jassert (pDitherEnabled != nullptr);
    pDitherBitDepth        = apvts.getRawParameterValue (ParamID::ditherBitDepth);
    jassert (pDitherBitDepth != nullptr);
    pDitherNoiseShaping    = apvts.getRawParameterValue (ParamID::ditherNoiseShaping);
    jassert (pDitherNoiseShaping != nullptr);
    pBypass                = apvts.getRawParameterValue (ParamID::bypass);
    jassert (pBypass != nullptr);
    pUnityGainMode         = apvts.getRawParameterValue (ParamID::unityGainMode);
    jassert (pUnityGainMode != nullptr);
    pSidechainHPFreq       = apvts.getRawParameterValue (ParamID::sidechainHPFreq);
    jassert (pSidechainHPFreq != nullptr);
    pSidechainLPFreq       = apvts.getRawParameterValue (ParamID::sidechainLPFreq);
    jassert (pSidechainLPFreq != nullptr);
    pSidechainTilt         = apvts.getRawParameterValue (ParamID::sidechainTilt);
    jassert (pSidechainTilt != nullptr);
    pDelta                 = apvts.getRawParameterValue (ParamID::delta);
    jassert (pDelta != nullptr);
}

void MLIMAudioProcessor::pushAllParametersToEngine()
{
    limiterEngine.setInputGain            (pInputGain->load());
    limiterEngine.setOutputCeiling        (pOutputCeiling->load());
    limiterEngine.setAlgorithm            (static_cast<LimiterAlgorithm> (static_cast<int> (pAlgorithm->load())));
    limiterEngine.setLookahead            (pLookahead->load());
    limiterEngine.setAttack               (pAttack->load());
    limiterEngine.setRelease              (pRelease->load());
    limiterEngine.setChannelLinkTransients(pChannelLinkTransients->load());
    limiterEngine.setChannelLinkRelease   (pChannelLinkRelease->load());
    limiterEngine.setTruePeakEnabled      (pTruePeakEnabled->load() >= 0.5f);
    limiterEngine.setDCFilterEnabled      (pDCFilterEnabled->load() >= 0.5f);
    limiterEngine.setDitherEnabled        (pDitherEnabled->load() >= 0.5f);
    limiterEngine.setDitherBitDepth       (16 + static_cast<int> (pDitherBitDepth->load()) * 2);
    limiterEngine.setDitherNoiseShaping   (static_cast<int> (pDitherNoiseShaping->load()));
    limiterEngine.setBypass               (pBypass->load() >= 0.5f);
    limiterEngine.setUnityGain            (pUnityGainMode->load() >= 0.5f);
    limiterEngine.setSidechainHPFreq      (pSidechainHPFreq->load());
    limiterEngine.setSidechainLPFreq      (pSidechainLPFreq->load());
    limiterEngine.setSidechainTilt        (pSidechainTilt->load());
    limiterEngine.setDeltaMode            (pDelta->load() >= 0.5f);
}

void MLIMAudioProcessor::updateLatency()
{
    setLatencySamples (limiterEngine.getLatencySamples());
}

// ---------------------------------------------------------------------------
// Plugin entry point
// ---------------------------------------------------------------------------

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MLIMAudioProcessor();
}
