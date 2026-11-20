#include "BandPanel.h"
#include "Theme.h"
#include "DSP.h"
#include <cmath>

using namespace lunar;

BandPanel::BandPanel (LunarEQAudioProcessor& p) : proc (p), shapes (p)
{
    for (auto* k : { &freqKnob, &gainKnob, &qKnob })
    {
        k->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k->setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                juce::MathConstants<float>::pi * 2.8f, true);
        addAndMakeVisible (k);
    }
    freqKnob.textFromValueFunction = [] (double v) { return formatFreqHz ((float) v); };
    gainKnob.textFromValueFunction = [] (double v) { return formatGain ((float) v) + " dB"; };
    qKnob.textFromValueFunction    = [] (double v) { return formatQ ((float) v); };

    typeBox.addItemList (typeNames(), 1);
    modeBox.addItemList (modeNames(), 1);
    slopeBox.addItemList (slopeNames(), 1);
    addAndMakeVisible (typeBox);
    addAndMakeVisible (modeBox);
    addAndMakeVisible (slopeBox);
    addAndMakeVisible (shapes);

    addAndMakeVisible (onBtn);
    addAndMakeVisible (soloBtn);
    addAndMakeVisible (removeBtn);

    // Eliminar: diferido con callAsync para no destruir attachments dentro de su propio callback
    juce::Component::SafePointer<BandPanel> safe (this);
    removeBtn.onClick = [safe]()
    {
        if (safe == nullptr)
            return;

        const int b = safe->band;
        juce::MessageManager::callAsync ([safe, b]()
        {
            if (safe != nullptr && safe->onRemove)
                safe->onRemove (b);
        });
    };

    setBand (-1);
}

void BandPanel::clearAttachments()
{
    freqAtt.reset(); gainAtt.reset(); qAtt.reset();
    onAtt.reset(); soloAtt.reset();
    typeAtt.reset(); modeAtt.reset(); slopeAtt.reset();
}

void BandPanel::buildAttachments()
{
    freqAtt  = std::make_unique<SliderAtt> (proc.apvts, paramId (band, "freq"), freqKnob);
    gainAtt  = std::make_unique<SliderAtt> (proc.apvts, paramId (band, "gain"), gainKnob);
    qAtt     = std::make_unique<SliderAtt> (proc.apvts, paramId (band, "q"), qKnob);
    onAtt    = std::make_unique<ButtonAtt> (proc.apvts, paramId (band, "on"), onBtn);
    soloAtt  = std::make_unique<ButtonAtt> (proc.apvts, paramId (band, "solo"), soloBtn);
    typeAtt  = std::make_unique<ComboAtt> (proc.apvts, paramId (band, "type"), typeBox);
    modeAtt  = std::make_unique<ComboAtt> (proc.apvts, paramId (band, "mode"), modeBox);
    slopeAtt = std::make_unique<ComboAtt> (proc.apvts, paramId (band, "slope"), slopeBox);

    const auto col = theme::bandColour (band);
    freqKnob.setColour (juce::Slider::rotarySliderFillColourId, col);
    gainKnob.setColour (juce::Slider::rotarySliderFillColourId, col);
    qKnob.setColour (juce::Slider::rotarySliderFillColourId, col);
    onBtn.setColour (juce::ToggleButton::tickColourId, theme::turquoise());
    soloBtn.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xffffd166));
}

void BandPanel::setControlsVisible (bool v)
{
    onBtn.setVisible (v);
    soloBtn.setVisible (v);
    removeBtn.setVisible (v);
    typeBox.setVisible (v);
    modeBox.setVisible (v);
    slopeBox.setVisible (false);
    shapes.setVisible (v);
    freqKnob.setVisible (v);
    gainKnob.setVisible (v);
    qKnob.setVisible (v);
}

void BandPanel::setBand (int b)
{
    band = b;
    clearAttachments();

    if (band >= 0)
        buildAttachments();

    shapes.setBand (band);
    setControlsVisible (band >= 0);
    refresh();
    repaint();
}

void BandPanel::refresh()
{
    if (band < 0)
        return;

    const int type = (int) std::round (proc.apvts.getRawParameterValue (paramId (band, "type"))->load());
    const bool hasGain = typeHasGain (type);
    const bool hasQ = typeUsesQ (type);

    gainKnob.setEnabled (hasGain);
    gainKnob.setAlpha (hasGain ? 1.0f : 0.3f);
    qKnob.setEnabled (hasQ);
    qKnob.setAlpha (hasQ ? 1.0f : 0.3f);
    slopeBox.setVisible (typeIsCut (type));

    repaint();
}

void BandPanel::paint (juce::Graphics& g)
{
    using namespace theme;
    drawSectionHeader (g, headLeft, band >= 0 ? "FILTRO" : "FILTRO");
    drawSectionHeader (g, headMid, "PARÁMETROS");

    if (band < 0)
    {
        g.setColour (textDim());
        g.setFont (fonts::inter (13.0f));
        g.drawFittedText ("Doble clic en el gráfico para añadir un filtro.\nSelecciona un nodo para editarlo.",
                          getLocalBounds().withTrimmedTop (44), juce::Justification::centred, 3);
        return;
    }

    // Número de banda
    const auto col = bandColour (band);
    g.setColour (col);
    g.fillEllipse (badgeArea.toFloat());
    g.setColour (juce::Colour (0xff14151f));
    g.setFont (fonts::jost (14.0f, 2));
    g.drawText (juce::String (band + 1), badgeArea, juce::Justification::centred, false);

    // Valores y rótulos de los knobs
    const juce::Slider* knobs[3] = { &freqKnob, &gainKnob, &qKnob };
    const char* caps[3] = { "FRECUENCIA", "GANANCIA", "Q" };
    for (int i = 0; i < 3; ++i)
    {
        g.setColour (knobs[i]->isEnabled() ? text() : textDim());
        g.setFont (fonts::inter (12.0f, true));
        g.drawText (knobs[i]->getTextFromValue (knobs[i]->getValue()), valArea[i], juce::Justification::centred, false);

        g.setColour (textDim());
        g.setFont (fonts::jost (9.0f, 1).withExtraKerningFactor (0.24f));
        g.drawText (caps[i], capArea[i], juce::Justification::centred, false);
    }
}

void BandPanel::resized()
{
    auto r = getLocalBounds();
    const int leftW = 430, gap = 36;

    auto leftCol = r.removeFromLeft (leftW);
    r.removeFromLeft (gap);
    auto midCol = r;

    headLeft = leftCol.removeFromTop (16);
    headMid  = midCol.removeFromTop (16);
    leftCol.removeFromTop (16);
    midCol.removeFromTop (16);

    // FILTRO
    auto row1 = leftCol.removeFromTop (34);
    badgeArea = row1.removeFromLeft (30).withSizeKeepingCentre (30, 30);
    row1.removeFromLeft (10);
    typeBox.setBounds (row1.removeFromLeft (140));
    row1.removeFromLeft (10);
    modeBox.setBounds (row1.removeFromLeft (120));
    row1.removeFromLeft (10);
    slopeBox.setBounds (row1.removeFromLeft (100));

    leftCol.removeFromTop (14);
    shapes.setBounds (leftCol.removeFromTop (32).removeFromLeft (7 * 44 - 6));

    leftCol.removeFromTop (14);
    auto row3 = leftCol.removeFromTop (30);
    onBtn.setBounds (row3.removeFromLeft (82));
    row3.removeFromLeft (8);
    soloBtn.setBounds (row3.removeFromLeft (92));
    row3.removeFromLeft (8);
    removeBtn.setBounds (row3.removeFromLeft (92));

    // PARÁMETROS: tres knobs con valor y rótulo
    const int cw = midCol.getWidth() / 3;
    juce::Slider* knobs[3] = { &freqKnob, &gainKnob, &qKnob };
    for (int i = 0; i < 3; ++i)
    {
        auto cell = midCol.removeFromLeft (cw);
        knobs[i]->setBounds (cell.removeFromTop (72).withSizeKeepingCentre (72, 72));
        valArea[i] = cell.removeFromTop (18);
        capArea[i] = cell.removeFromTop (14);
    }
}
