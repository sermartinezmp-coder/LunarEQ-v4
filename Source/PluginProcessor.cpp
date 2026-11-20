#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace lunar;

namespace
{
// Frecuencias por defecto de los 16 slots, repartidas en escala logarítmica
float defaultFreq (int b)
{
    return 30.0f * std::pow (16000.0f / 30.0f, (float) b / (float) (kNumBands - 1));
}
}

LunarEQAudioProcessor::LunarEQAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "LUNAR_EQ_STATE", createLayout())
{
    for (int b = 0; b < kNumBands; ++b)
    {
        pOn[b]    = apvts.getRawParameterValue (paramId (b, "on"));
        pType[b]  = apvts.getRawParameterValue (paramId (b, "type"));
        pFreq[b]  = apvts.getRawParameterValue (paramId (b, "freq"));
        pGain[b]  = apvts.getRawParameterValue (paramId (b, "gain"));
        pQ[b]     = apvts.getRawParameterValue (paramId (b, "q"));
        pSlope[b] = apvts.getRawParameterValue (paramId (b, "slope"));
        pMode[b]  = apvts.getRawParameterValue (paramId (b, "mode"));
        pSolo[b]  = apvts.getRawParameterValue (paramId (b, "solo"));
    }
    inGainParam   = apvts.getRawParameterValue ("in_gain");
    outGainParam  = apvts.getRawParameterValue ("out_gain");
    mixParam      = apvts.getRawParameterValue ("mix");
    analyzerParam = apvts.getRawParameterValue ("analyzer");
    outModeParam  = apvts.getRawParameterValue ("out_mode");
    outBarParam   = apvts.getRawParameterValue ("out_bar");
}

juce::AudioProcessorValueTreeState::ParameterLayout LunarEQAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto gainAttr = [] (const juce::String& unit)
    {
        return juce::AudioParameterFloatAttributes()
            .withLabel (unit)
            .withStringFromValueFunction ([] (float v, int) { return formatGain (v); })
            .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });
    };

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "in_gain", 1 }, "Input Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f, gainAttr ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "out_gain", 1 }, "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f, gainAttr ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes()
            .withLabel ("%")
            .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 0) + " %"; })
            .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); })));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "analyzer", 1 }, "Analyzer", analyzerNames(), 3));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "out_mode", 1 }, "Output Mode", juce::StringArray { "L/R", "M/S" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "out_bar", 1 }, "Output Balance/Width",
        juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f));

    for (int b = 0; b < kNumBands; ++b)
    {
        const juce::String n = "Band " + juce::String (b + 1) + " ";

        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { paramId (b, "on"), 1 }, n + "On", false));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { paramId (b, "type"), 1 }, n + "Type", typeNames(), (int) Bell));

        juce::NormalisableRange<float> fr (kMinFreq, kMaxFreq, 0.01f);
        fr.setSkewForCentre (1000.0f);
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { paramId (b, "freq"), 1 }, n + "Freq", fr, defaultFreq (b),
            juce::AudioParameterFloatAttributes()
                .withLabel ("Hz")
                .withStringFromValueFunction ([] (float v, int) { return formatFreq (v); })
                .withValueFromStringFunction ([] (const juce::String& t) { return parseFreq (t); })));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { paramId (b, "gain"), 1 }, n + "Gain",
            juce::NormalisableRange<float> (-kMaxGainDb, kMaxGainDb, 0.01f), 0.0f, gainAttr ("dB")));

        juce::NormalisableRange<float> qr (0.1f, 18.0f, 0.001f);
        qr.setSkewForCentre (1.0f);
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { paramId (b, "q"), 1 }, n + "Q", qr, 1.0f,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([] (float v, int) { return formatQ (v); })
                .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); })));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { paramId (b, "slope"), 1 }, n + "Slope", slopeNames(), 1));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { paramId (b, "mode"), 1 }, n + "Mode", modeNames(), 0));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { paramId (b, "solo"), 1 }, n + "Solo", false));
    }

    return layout;
}

BandSettings LunarEQAudioProcessor::getBandSettings (int b) const
{
    BandSettings s;
    s.on    = pOn[b]->load() > 0.5f;
    s.type  = juce::jlimit (0, (int) NumBandTypes - 1, (int) std::round (pType[b]->load()));
    s.freq  = pFreq[b]->load();
    s.gain  = pGain[b]->load();
    s.q     = pQ[b]->load();
    s.slope = juce::jlimit (0, 3, (int) std::round (pSlope[b]->load()));
    s.mode  = juce::jlimit (0, (int) NumModes - 1, (int) std::round (pMode[b]->load()));
    s.solo  = pSolo[b]->load() > 0.5f;
    return s;
}

int LunarEQAudioProcessor::getAnalyzerMode() const
{
    return juce::jlimit (0, 3, (int) std::round (analyzerParam->load()));
}

int LunarEQAudioProcessor::getOutputMode() const
{
    return outModeParam->load() > 0.5f ? 1 : 0;
}

float LunarEQAudioProcessor::getOutputBar() const
{
    return outBarParam->load();
}

void LunarEQAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate > 0.0 ? sampleRate : 48000.0;
    bandCoef = 1.0f - std::exp (-(float) kChunk / (0.02f * (float) sr));
    gainCoef = 1.0f - std::exp (-1.0f / (0.02f * (float) sr));
    for (auto& b : bands)
        b = BandRuntime();
    soloIndex = -1;
    curIn  = juce::Decibels::decibelsToGain (inGainParam->load());
    curOut = juce::Decibels::decibelsToGain (outGainParam->load());
    analyzerFifo.reset();
}

bool LunarEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void LunarEQAudioProcessor::updateBands()
{
    soloIndex = -1;

    for (int b = 0; b < kNumBands; ++b)
    {
        auto& rt = bands[(size_t) b];
        const auto target = getBandSettings (b);

        if (target.on && target.solo && soloIndex < 0)
            soloIndex = b;

        if (! target.on)
        {
            if (rt.active)
            {
                rt.active = false;
                for (auto& ch : rt.st)
                    for (auto& s : ch)
                        s = BiquadState();
            }
            continue;
        }

        const float tf = std::log (juce::jlimit (kMinFreq, kMaxFreq, target.freq));
        if (! rt.active)
        {
            rt.fLog = tf;
            rt.gain = target.gain;
            rt.q    = target.q;
            for (auto& ch : rt.st)
                for (auto& s : ch)
                    s = BiquadState();
            rt.active = true;
        }
        else
        {
            rt.fLog += (tf - rt.fLog) * bandCoef;
            rt.gain += (target.gain - rt.gain) * bandCoef;
            rt.q    += (target.q - rt.q) * bandCoef;
        }

        rt.mode = target.mode;

        BandSettings s = target;
        s.freq = std::exp (rt.fLog);
        s.gain = rt.gain;
        s.q    = rt.q;
        rt.stages = designBand (s, sr, rt.c);
    }
}

float LunarEQAudioProcessor::runChain (BandRuntime& rt, int idx, float x)
{
    float y = x;
    for (int s = 0; s < rt.stages; ++s)
        y = processSample (rt.c[s], rt.st[idx][s], y);
    return y;
}

// Aplica una banda sobre L/R según el modo. Para M y S se procesa la señal
// transformada y se reconstruye la diferencia (delta) en L y R.
void LunarEQAudioProcessor::processBand (BandRuntime& rt, int mode, float& l, float& r)
{
    switch (mode)
    {
        case ModeLeft:
        {
            const float d = runChain (rt, 0, l) - l;
            l += d;
            break;
        }
        case ModeRight:
        {
            const float d = runChain (rt, 1, r) - r;
            r += d;
            break;
        }
        case ModeMid:
        {
            const float m = 0.5f * (l + r);
            const float d = runChain (rt, 2, m) - m;
            l += d;
            r += d;
            break;
        }
        case ModeSide:
        {
            const float sd = 0.5f * (l - r);
            const float d = runChain (rt, 3, sd) - sd;
            l += d;
            r -= d;
            break;
        }
        default:
            l = runChain (rt, 0, l);
            r = runChain (rt, 1, r);
            break;
    }
}

void LunarEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numSamples == 0 || buffer.getNumChannels() == 0)
        return;

    const bool stereo = buffer.getNumChannels() > 1;
    float* L = buffer.getWritePointer (0);
    float* R = stereo ? buffer.getWritePointer (1) : nullptr;

    const float targetIn  = juce::Decibels::decibelsToGain (inGainParam->load());
    const float targetOut = juce::Decibels::decibelsToGain (outGainParam->load());
    const float mix = juce::jlimit (0.0f, 1.0f, mixParam->load() * 0.01f);

    const int toWrite = juce::jmin (numSamples, analyzerFifo.getFreeSpace());
    int s1 = 0, n1 = 0, s2 = 0, n2 = 0;
    if (toWrite > 0)
        analyzerFifo.prepareToWrite (toWrite, s1, n1, s2, n2);

    for (int i = 0; i < numSamples; ++i)
    {
        if (i % kChunk == 0)
            updateBands();

        curIn  += (targetIn - curIn) * gainCoef;
        curOut += (targetOut - curOut) * gainCoef;

        float l = L[i] * curIn;
        float r = stereo ? R[i] * curIn : l;
        const float dryL = l, dryR = r;

        float soloL = 0.0f, soloR = 0.0f;
        const int solo = soloIndex;

        for (int b = 0; b < kNumBands; ++b)
        {
            auto& rt = bands[(size_t) b];
            if (! rt.active)
                continue;

            const float pl = l, pr = r;
            processBand (rt, rt.mode, l, r);

            if (b == solo)
            {
                soloL += l - pl;
                soloR += r - pr;
            }
        }

        float outL, outR;
        if (solo >= 0)
        {
            outL = soloL;
            outR = soloR;
        }
        else
        {
            outL = dryL + (l - dryL) * mix;
            outR = dryR + (r - dryR) * mix;
        }

        // Matriz de salida (post-EQ): balance en L/R o anchura en M/S
        if (solo < 0 && stereo)
        {
            const float bar = outBarParam->load() * 0.01f;
            if (outModeParam->load() < 0.5f)
            {
                outL *= juce::jlimit (0.0f, 1.0f, 1.0f - bar);
                outR *= juce::jlimit (0.0f, 1.0f, 1.0f + bar);
            }
            else
            {
                const float m = 0.5f * (outL + outR);
                const float s = 0.5f * (outL - outR) * (1.0f + bar);
                outL = m + s;
                outR = m - s;
            }
        }

        L[i] = outL * curOut;
        if (stereo)
            R[i] = outR * curOut;

        if (i < toWrite)
        {
            const int idx = (i < n1) ? s1 + i : s2 + (i - n1);
            preBuffer[(size_t) idx]  = 0.5f * (dryL + dryR);
            postBuffer[(size_t) idx] = 0.5f * (l + r);
        }
    }

    if (toWrite > 0)
        analyzerFifo.finishedWrite (toWrite);
}

juce::AudioProcessorEditor* LunarEQAudioProcessor::createEditor()
{
    return new LunarEQAudioProcessorEditor (*this);
}

void LunarEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void LunarEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LunarEQAudioProcessor();
}
