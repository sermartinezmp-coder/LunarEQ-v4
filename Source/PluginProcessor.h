#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include "DSP.h"

class LunarEQAudioProcessor : public juce::AudioProcessor
{
public:
    LunarEQAudioProcessor();
    ~LunarEQAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "LUNAR EQ"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    lunar::BandSettings getBandSettings (int band) const;
    int getAnalyzerMode() const;   // 0 apagado, 1 pre, 2 post, 3 pre+post
    int getOutputMode() const;     // 0 L/R, 1 M/S
    float getOutputBar() const;    // -100..100 (balance o anchura)

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;

    // Buffers mono pre y post para el analizador (la GUI los lee por FIFO)
    static constexpr int kFifoSize = 16384;
    juce::AbstractFifo analyzerFifo { kFifoSize };
    std::array<float, kFifoSize> preBuffer {};
    std::array<float, kFifoSize> postBuffer {};

private:
    static constexpr int kChunk = 32;

    struct BandRuntime
    {
        bool  active = false;
        int   mode = 0;
        float fLog = 0.0f, gain = 0.0f, q = 1.0f;
        int   stages = 1;
        lunar::Coeffs c[lunar::kMaxStages];
        lunar::BiquadState st[4][lunar::kMaxStages];   // 0=L 1=R 2=M 3=S
    };

    void updateBands();
    void processBand (BandRuntime& rt, int mode, float& l, float& r);
    static float runChain (BandRuntime& rt, int idx, float x);

    std::array<BandRuntime, lunar::kNumBands> bands;
    int soloIndex = -1;

    std::atomic<float>* pOn[lunar::kNumBands]    {};
    std::atomic<float>* pType[lunar::kNumBands]  {};
    std::atomic<float>* pFreq[lunar::kNumBands]  {};
    std::atomic<float>* pGain[lunar::kNumBands]  {};
    std::atomic<float>* pQ[lunar::kNumBands]     {};
    std::atomic<float>* pSlope[lunar::kNumBands] {};
    std::atomic<float>* pMode[lunar::kNumBands]  {};
    std::atomic<float>* pSolo[lunar::kNumBands]  {};
    std::atomic<float>* inGainParam  = nullptr;
    std::atomic<float>* outGainParam = nullptr;
    std::atomic<float>* mixParam     = nullptr;
    std::atomic<float>* analyzerParam = nullptr;
    std::atomic<float>* outModeParam = nullptr;
    std::atomic<float>* outBarParam  = nullptr;

    double sr = 48000.0;
    float bandCoef = 0.2f;
    float gainCoef = 0.01f;
    float curIn = 1.0f, curOut = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LunarEQAudioProcessor)
};
