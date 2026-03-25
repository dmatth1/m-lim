#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "dsp/LimiterEngine.h"
#include "dsp/LoudnessMeter.h"
#include "dsp/MeterData.h"
#include "state/ABState.h"
#include "state/PresetManager.h"
#include "state/UndoManager.h"

class MLIMAudioProcessor : public juce::AudioProcessor,
                           private juce::AudioProcessorValueTreeState::Listener,
                           private juce::AsyncUpdater
{
public:
    MLIMAudioProcessor();
    ~MLIMAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>& buffer,
                       juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // State management — must be constructed before apvts (construction order = declaration order)
    UndoManager   undoManager;
    ABState       abState;
    PresetManager presetManager;

    // Depends on undoManager being alive (apvts ctor takes &undoManager.getJuceUndoManager())
    juce::AudioProcessorValueTreeState apvts;

    // Meter FIFO — audio thread pushes MeterData (with LUFS), UI thread pops
    LockFreeFIFO<MeterData>& getMeterFIFO() { return mProcessorMeterFIFO; }

    // Loudness meter — read from any thread (results are atomic)
    const LoudnessMeter& getLoudnessMeter() const noexcept { return loudnessMeter; }

private:
    LimiterEngine limiterEngine;
    LoudnessMeter loudnessMeter;

    // Processor-owned FIFO: audio thread drains engine FIFO, augments with LUFS, pushes here
    LockFreeFIFO<MeterData> mProcessorMeterFIFO { 32 };

    // Raw parameter pointers — initialised once in constructor, read on audio thread
    std::atomic<float>* pInputGain             = nullptr;
    std::atomic<float>* pOutputCeiling         = nullptr;
    std::atomic<float>* pAlgorithm             = nullptr;
    std::atomic<float>* pLookahead             = nullptr;
    std::atomic<float>* pAttack                = nullptr;
    std::atomic<float>* pRelease               = nullptr;
    std::atomic<float>* pChannelLinkTransients = nullptr;
    std::atomic<float>* pChannelLinkRelease    = nullptr;
    std::atomic<float>* pTruePeakEnabled       = nullptr;
    std::atomic<float>* pOversamplingFactor    = nullptr;
    std::atomic<float>* pDCFilterEnabled       = nullptr;
    std::atomic<float>* pDitherEnabled         = nullptr;
    std::atomic<float>* pDitherBitDepth        = nullptr;
    std::atomic<float>* pDitherNoiseShaping    = nullptr;
    std::atomic<float>* pBypass                = nullptr;
    std::atomic<float>* pUnityGainMode         = nullptr;
    std::atomic<float>* pSidechainHPFreq       = nullptr;
    std::atomic<float>* pSidechainLPFreq       = nullptr;
    std::atomic<float>* pSidechainTilt         = nullptr;
    std::atomic<float>* pDelta                 = nullptr;
    std::atomic<float>* pDisplayMode          = nullptr;

    // Oversampling factor changes require reallocation; defer to prepareToPlay
    std::atomic<bool>  mOversamplingChangePending { false };
    std::atomic<int>   mPendingOversamplingFactor { 0 };
    int                mAppliedOversamplingFactor = -1;

    // -----------------------------------------------------------------------
    // APVTS::Listener — called on message thread when params change
    // -----------------------------------------------------------------------
    void parameterChanged (const juce::String& paramID, float newValue) override;

    // -----------------------------------------------------------------------
    // AsyncUpdater — applies deferred oversampling changes on the message thread
    // -----------------------------------------------------------------------
    void handleAsyncUpdate() override;

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------
    void initParameterPointers();
    void pushAllParametersToEngine();
    void updateLatency();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MLIMAudioProcessor)
};
