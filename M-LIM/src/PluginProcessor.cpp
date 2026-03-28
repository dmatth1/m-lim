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

    // Listen for parameter changes that need special handling (oversampling, lookahead)
    apvts.addParameterListener (ParamID::oversamplingFactor, this);
    apvts.addParameterListener (ParamID::lookahead,         this);
}

MLIMAudioProcessor::~MLIMAudioProcessor()
{
    apvts.removeParameterListener (ParamID::oversamplingFactor, this);
    apvts.removeParameterListener (ParamID::lookahead,         this);
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

    // Push all parameter values to the engine (real-time safe atomic reads).
    // NOTE: oversamplingFactor changes are deferred — do NOT call setOversamplingFactor
    // here (it allocates memory). Changes are handled via parameterChanged + prepareToPlay.
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
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml != nullptr)
        copyXmlToBinary (*xml, destData);
}

void MLIMAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
        {
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
    if (pInputGain)             limiterEngine.setInputGain            (pInputGain->load());
    if (pOutputCeiling)         limiterEngine.setOutputCeiling        (pOutputCeiling->load());
    if (pAlgorithm)             limiterEngine.setAlgorithm            (static_cast<LimiterAlgorithm> (static_cast<int> (pAlgorithm->load())));
    if (pLookahead)             limiterEngine.setLookahead            (pLookahead->load());
    if (pAttack)                limiterEngine.setAttack               (pAttack->load());
    if (pRelease)               limiterEngine.setRelease              (pRelease->load());
    if (pChannelLinkTransients) limiterEngine.setChannelLinkTransients(pChannelLinkTransients->load());
    if (pChannelLinkRelease)    limiterEngine.setChannelLinkRelease   (pChannelLinkRelease->load());
    if (pTruePeakEnabled)       limiterEngine.setTruePeakEnabled      (pTruePeakEnabled->load() >= 0.5f);
    if (pDCFilterEnabled)       limiterEngine.setDCFilterEnabled      (pDCFilterEnabled->load() >= 0.5f);
    if (pDitherEnabled)         limiterEngine.setDitherEnabled        (pDitherEnabled->load() >= 0.5f);
    if (pDitherBitDepth)        limiterEngine.setDitherBitDepth       (16 + static_cast<int> (pDitherBitDepth->load()) * 2);
    if (pDitherNoiseShaping)    limiterEngine.setDitherNoiseShaping   (static_cast<int> (pDitherNoiseShaping->load()));
    if (pBypass)                limiterEngine.setBypass               (pBypass->load() >= 0.5f);
    if (pUnityGainMode)         limiterEngine.setUnityGain            (pUnityGainMode->load() >= 0.5f);
    if (pSidechainHPFreq)       limiterEngine.setSidechainHPFreq      (pSidechainHPFreq->load());
    if (pSidechainLPFreq)       limiterEngine.setSidechainLPFreq      (pSidechainLPFreq->load());
    if (pSidechainTilt)         limiterEngine.setSidechainTilt        (pSidechainTilt->load());
    if (pDelta)                 limiterEngine.setDeltaMode            (pDelta->load() >= 0.5f);
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
