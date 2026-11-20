#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <functional>
#include <vector>
#include "PluginProcessor.h"
#include "NodePopup.h"

// Gráfico principal: analizador pre/post, curva de EQ y nodos
class GraphComponent : public juce::Component, private juce::Timer
{
public:
    explicit GraphComponent (LunarEQAudioProcessor& p);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    void setSelectedBand (int band);
    int  getSelectedBand() const { return selected; }
    void removeBand (int band);

    std::function<void (int)> onSelectedChanged;

private:
    static constexpr int kFftOrder = 12;
    static constexpr int kFftSize  = 1 << kFftOrder;
    static constexpr int kBins     = kFftSize / 2;
    static constexpr int kTmp      = LunarEQAudioProcessor::kFifoSize;

    struct SpecState
    {
        std::vector<float> slide = std::vector<float> ((size_t) kFftSize, 0.0f);
        std::vector<float> data  = std::vector<float> ((size_t) kFftSize * 2, 0.0f);
        std::vector<float> db    = std::vector<float> ((size_t) kBins, -120.0f);
    };

    void timerCallback() override;
    void updateSpectrum();
    void pushSlide (SpecState& s, const float* src, int n) const;
    void computeSpectrum (SpecState& s);
    void drawSpectrum (juce::Graphics& g, juce::Rectangle<float> p, const SpecState& s, bool filled, bool thinStyle) const;
    void drawGrid (juce::Graphics& g, juce::Rectangle<float> p) const;
    void drawCurves (juce::Graphics& g, juce::Rectangle<float> p, double sr) const;
    void drawNodes (juce::Graphics& g) const;

    double sampleRateOr() const;
    juce::Rectangle<float> plot() const;
    juce::Rectangle<float> rangeChip() const;
    float freqToX (float f) const;
    float xToFreq (float x) const;
    float dbToY (float db) const;
    float yToDb (float y) const;
    juce::Point<float> nodePos (int band) const;
    int hitTestNode (juce::Point<float> pt) const;

    void beginGesture (const juce::String& id);
    void endGesture (const juce::String& id);
    void setParam (const juce::String& id, float realValue);
    void setParamGesture (const juce::String& id, float realValue);
    void showBandMenu (int band);
    void layoutPopup();
    void addBandAt (juce::Point<float> pos);

    LunarEQAudioProcessor& proc;
    NodePopup popup;
    float range = 12.0f;   // rango visible de la escala de ganancia (dB)
    int selected = -1, hover = -1, dragBand = -1;
    bool dragHasGain = false;

    juce::dsp::FFT fft { kFftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) kFftSize, juce::dsp::WindowingFunction<float>::hann, false };
    SpecState preS, postS;
    std::vector<float> tmpPre = std::vector<float> ((size_t) kTmp, 0.0f);
    std::vector<float> tmpPost = std::vector<float> ((size_t) kTmp, 0.0f);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GraphComponent)
};
