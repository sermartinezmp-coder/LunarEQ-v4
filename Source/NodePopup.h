#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include "PluginProcessor.h"
#include "DSP.h"
#include "Theme.h"
#include "IconButton.h"

// Recuadro que aparece junto al nodo seleccionado: info, encendido, escucha y forma
class NodePopup : public juce::Component
{
public:
    explicit NodePopup (LunarEQAudioProcessor& p) : proc (p)
    {
        setSize (226, 38);
        addAndMakeVisible (powerBtn);
        addAndMakeVisible (listenBtn);
        addAndMakeVisible (shapeBtn);
        shapeBtn.onClick = [this] { showTypeMenu(); };
    }

    void setBand (int b)
    {
        band = b;
        onAtt.reset();
        listenAtt.reset();
        if (band >= 0)
        {
            onAtt     = std::make_unique<ButtonAtt> (proc.apvts, lunar::paramId (band, "on"), powerBtn);
            listenAtt = std::make_unique<ButtonAtt> (proc.apvts, lunar::paramId (band, "solo"), listenBtn);
            refreshInfo();
        }
    }

    int getBand() const { return band; }

    void refreshInfo()
    {
        if (band < 0)
            return;
        const auto s = proc.getBandSettings (band);
        freqText = lunar::formatFreqHz (s.freq);
        gainText = lunar::typeHasGain (s.type) ? lunar::formatGain (s.gain) + " dB" : juce::String();
        shapeBtn.setShape (s.type);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using namespace lunar::theme;
        auto b = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (juce::Colour (0xf0171a2a));
        g.fillRoundedRectangle (b, 13.0f);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawRoundedRectangle (b, 13.0f, 1.0f);

        auto font = fonts::inter (12.5f, true);
        g.setFont (font);
        g.setColour (text());
        const float fw = textWidth (font, freqText);
        g.drawText (freqText, juce::Rectangle<float> (14.0f, 0.0f, fw + 4.0f, (float) getHeight()),
                    juce::Justification::centredLeft, false);
        g.setColour (lavender());
        g.drawText (gainText, juce::Rectangle<float> (14.0f + fw + 10.0f, 0.0f, 64.0f, (float) getHeight()),
                    juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        const int s = 30;
        powerBtn.setBounds (getWidth() - 3 * s - 14, (getHeight() - s) / 2, s, s);
        listenBtn.setBounds (getWidth() - 2 * s - 12, (getHeight() - s) / 2, s, s);
        shapeBtn.setBounds (getWidth() - s - 10, (getHeight() - s) / 2, s, s);
    }

private:
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void showTypeMenu()
    {
        if (band < 0)
            return;
        juce::PopupMenu m;
        const auto names = lunar::typeNames();
        const int cur = proc.getBandSettings (band).type;
        for (int i = 0; i < names.size(); ++i)
            m.addItem (i + 1, names[i], true, cur == i);

        const int b = band;
        auto* procPtr = &proc;
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&shapeBtn),
                         [procPtr, b] (int r)
                         {
                             if (r == 0)
                                 return;
                             if (auto* prm = procPtr->apvts.getParameter (lunar::paramId (b, "type")))
                             {
                                 prm->beginChangeGesture();
                                 prm->setValueNotifyingHost (prm->convertTo0to1 ((float) (r - 1)));
                                 prm->endChangeGesture();
                             }
                         });
    }

    LunarEQAudioProcessor& proc;
    int band = -1;
    juce::String freqText, gainText;

    IconButton powerBtn { IconButton::Power };
    IconButton listenBtn { IconButton::Headphones };
    IconButton shapeBtn { IconButton::Shape };

    std::unique_ptr<ButtonAtt> onAtt, listenAtt;
};
