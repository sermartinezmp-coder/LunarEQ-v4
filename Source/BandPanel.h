#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <memory>
#include "PluginProcessor.h"
#include "Widgets.h"

// Zona inferior izquierda: FILTRO y PARÁMETROS del slot seleccionado
class BandPanel : public juce::Component
{
public:
    explicit BandPanel (LunarEQAudioProcessor& p);

    void setBand (int band);
    void refresh();
    void paint (juce::Graphics&) override;
    void resized() override;

    std::function<void (int)> onRemove;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void buildAttachments();
    void clearAttachments();
    void setControlsVisible (bool visible);

    LunarEQAudioProcessor& proc;
    int band = -1;

    FilterShapeRow shapes;
    juce::ToggleButton onBtn { "Activo" }, soloBtn { "Escucha" };
    juce::TextButton removeBtn { "Eliminar" };
    juce::ComboBox typeBox, modeBox, slopeBox;
    juce::Slider freqKnob, gainKnob, qKnob;

    juce::Rectangle<int> headLeft, headMid, badgeArea;
    juce::Rectangle<int> capArea[3], valArea[3];

    std::unique_ptr<SliderAtt> freqAtt, gainAtt, qAtt;
    std::unique_ptr<ButtonAtt> onAtt, soloAtt;
    std::unique_ptr<ComboAtt> typeAtt, modeAtt, slopeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BandPanel)
};
