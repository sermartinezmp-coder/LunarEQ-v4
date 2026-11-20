#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>
#include "PluginProcessor.h"
#include "GraphComponent.h"
#include "BandPanel.h"
#include "OutputSection.h"
#include "IconButton.h"
#include "Widgets.h"
#include "Theme.h"

class LunarEQAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LunarEQAudioProcessorEditor (LunarEQAudioProcessor&);
    ~LunarEQAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    void timerCallback() override;
    std::vector<float> currentValues() const;
    void commitIfIdle();
    void undo();
    void redo();
    void resetAll();
    void setParamValue (const juce::String& id, float value);

    void refreshUserPresets();
    void showPresetMenu();
    void handlePresetResult (int result);
    void applyFactory (int index);
    void applyPresetIndex (int index);
    void stepPreset (int delta);
    void saveUserPreset();
    void openLoadDialog();
    void loadUserPreset (const juce::File& file);

    LunarEQAudioProcessor& proc;
    lunar::theme::LunarLookAndFeel lnf;   // se declara antes que los componentes

    GraphComponent graph;
    BandPanel panel;
    OutputSection output;

    PresetPill presetPill;
    IconButton undoBtn { IconButton::Undo };
    IconButton redoBtn { IconButton::Redo };
    IconButton resetBtn { IconButton::Reset };
    ChoiceSegment analyzerSeg;

    juce::Image bgCache;
    juce::Rectangle<int> analyzerLabelArea;

    juce::StringArray factoryNames { "Plano", "Aire", "Calidez", "Graves limpios" };
    juce::Array<juce::File> userPresets;
    int presetIndex = 0;
    std::unique_ptr<juce::FileChooser> chooser;

    std::vector<juce::ValueTree> undoStack, redoStack;
    juce::ValueTree lastState;
    std::vector<float> lastValues;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LunarEQAudioProcessorEditor)
};
