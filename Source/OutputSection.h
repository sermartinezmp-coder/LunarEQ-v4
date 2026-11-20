#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include <memory>
#include "PluginProcessor.h"
#include "DSP.h"
#include "Theme.h"
#include "Widgets.h"

// SALIDA: knob de nivel + barra de balance (L/R) o anchura (M/S)
class OutputSection : public juce::Component
{
public:
    explicit OutputSection (LunarEQAudioProcessor& p)
        : proc (p), modeSeg (*p.apvts.getParameter ("out_mode"), juce::StringArray { "L/R", "M/S" }, 0, true)
    {
        knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        knob.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                  juce::MathConstants<float>::pi * 2.8f, true);
        knob.setColour (juce::Slider::rotarySliderFillColourId, lunar::theme::turquoise());

        bar.setSliderStyle (juce::Slider::LinearHorizontal);
        bar.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);

        addAndMakeVisible (knob);
        addAndMakeVisible (bar);
        addAndMakeVisible (modeSeg);

        knobAtt = std::make_unique<SliderAtt> (proc.apvts, "out_gain", knob);
        barAtt  = std::make_unique<SliderAtt> (proc.apvts, "out_bar", bar);
    }

    void refresh() { repaint(); }

    void paint (juce::Graphics& g) override
    {
        using namespace lunar::theme;
        drawSectionHeader (g, headerArea, "SALIDA");

        const bool ms = proc.getOutputMode() == 1;
        const float v = proc.getOutputBar();
        const float gain = proc.apvts.getRawParameterValue ("out_gain")->load();

        g.setColour (text());
        g.setFont (fonts::inter (12.0f, true));
        g.drawText (lunar::formatGain (gain) + " dB", knobValueArea, juce::Justification::centred, false);

        g.setColour (textDim());
        g.setFont (fonts::jost (9.0f, 1).withExtraKerningFactor (0.24f));
        g.drawText ("NIVEL", knobCapArea, juce::Justification::centred, false);
        g.drawText (ms ? "ANCHURA" : "BALANCE", balCapArea, juce::Justification::centredLeft, false);

        juce::String mid;
        if (ms)
            mid = juce::String (juce::roundToInt (100.0f + v)) + " %";
        else if (std::abs (v) < 0.5f)
            mid = "Centro";
        else
            mid = (v > 0.0f ? "R " : "L ") + juce::String (juce::roundToInt (std::abs (v)));

        g.setFont (fonts::inter (11.0f));
        g.setColour (textDim());
        g.drawText (ms ? "Mono" : "L", labelRowArea, juce::Justification::centredLeft, false);
        g.drawText (ms ? "Wide" : "R", labelRowArea, juce::Justification::centredRight, false);
        g.setColour (text());
        g.drawText (mid, labelRowArea, juce::Justification::centred, false);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        headerArea = r.removeFromTop (16);
        r.removeFromTop (16);

        auto left = r.removeFromLeft (84);
        knob.setBounds (left.removeFromTop (72).withSizeKeepingCentre (72, 72));
        knobValueArea = left.removeFromTop (18);
        knobCapArea = left.removeFromTop (14);

        r.removeFromLeft (22);
        auto top = r.removeFromTop (28);
        modeSeg.setBounds (top.removeFromRight (96));
        balCapArea = top;

        r.removeFromTop (14);
        bar.setBounds (r.removeFromTop (24));
        labelRowArea = r.removeFromTop (20);
    }

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;

    LunarEQAudioProcessor& proc;
    ChoiceSegment modeSeg;
    juce::Slider knob, bar;
    juce::Rectangle<int> headerArea, knobValueArea, knobCapArea, balCapArea, labelRowArea;
    std::unique_ptr<SliderAtt> knobAtt, barAtt;
};
