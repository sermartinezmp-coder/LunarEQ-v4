#include "PluginEditor.h"

using namespace lunar;

namespace
{
struct FactoryBand { int band; int type; float freq; float gain; float q; };

// Tipos: 0 campana, 1 low shelf, 2 high shelf, 3 low cut
std::vector<FactoryBand> factoryPreset (int index)
{
    switch (index)
    {
        case 1: return { { 0, 2, 10000.0f,  2.0f, 0.7f }, { 1, 0, 3000.0f, 1.2f, 0.8f } };
        case 2: return { { 0, 1,   120.0f,  1.5f, 0.7f }, { 1, 0,  300.0f, 1.0f, 0.9f },
                         { 2, 2,  9000.0f, -1.5f, 0.7f } };
        case 3: return { { 0, 3,    35.0f,  0.0f, 1.0f } };
        default: return {};
    }
}

juce::File presetDir()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("LunarEQ").getChildFile ("Presets");
    dir.createDirectory();
    return dir;
}

constexpr int kDefaultW = 1360, kDefaultH = 860;
}

LunarEQAudioProcessorEditor::LunarEQAudioProcessorEditor (LunarEQAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), graph (p), panel (p), output (p),
      analyzerSeg (*p.apvts.getParameter ("analyzer"), juce::StringArray { "Pre", "Post", "Pre + Post" }, 1)
{
    setLookAndFeel (&lnf);
    setSize (kDefaultW, kDefaultH);
    setResizable (true, true);
    setResizeLimits (1180, 740, 2600, 1500);
    setWantsKeyboardFocus (true);

    addAndMakeVisible (graph);
    addAndMakeVisible (panel);
    addAndMakeVisible (output);
    addAndMakeVisible (presetPill);
    addAndMakeVisible (undoBtn);
    addAndMakeVisible (redoBtn);
    addAndMakeVisible (resetBtn);
    addAndMakeVisible (analyzerSeg);

    graph.onSelectedChanged = [this] (int b) { panel.setBand (b); };
    panel.onRemove = [this] (int b) { graph.removeBand (b); };

    presetPill.setName (factoryNames[0]);
    presetPill.onMenu = [this] { showPresetMenu(); };
    presetPill.onPrev = [this] { stepPreset (-1); };
    presetPill.onNext = [this] { stepPreset (1); };

    undoBtn.onClick  = [this] { undo(); };
    redoBtn.onClick  = [this] { redo(); };
    resetBtn.onClick = [this] { resetAll(); };

    refreshUserPresets();
    lastState  = proc.apvts.copyState();
    lastValues = currentValues();

    panel.setBand (-1);
    startTimerHz (30);
}

LunarEQAudioProcessorEditor::~LunarEQAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

// ---------------------------------------------------------------- deshacer / rehacer
std::vector<float> LunarEQAudioProcessorEditor::currentValues() const
{
    std::vector<float> v;
    for (auto* prm : proc.getParameters())
        v.push_back (prm->getValue());
    return v;
}

// Guarda un paso cuando se suelta el ratón (no durante un arrastre)
void LunarEQAudioProcessorEditor::commitIfIdle()
{
    if (juce::ModifierKeys::getCurrentModifiers().isAnyMouseButtonDown())
        return;

    auto now = currentValues();
    if (now == lastValues)
        return;

    undoStack.push_back (lastState);
    if (undoStack.size() > 100)
        undoStack.erase (undoStack.begin());
    redoStack.clear();

    lastState  = proc.apvts.copyState();
    lastValues = now;
}

void LunarEQAudioProcessorEditor::undo()
{
    commitIfIdle();
    if (undoStack.empty())
        return;

    redoStack.push_back (lastState);
    auto prev = undoStack.back();
    undoStack.pop_back();

    proc.apvts.replaceState (prev);
    lastState  = prev;
    lastValues = currentValues();
}

void LunarEQAudioProcessorEditor::redo()
{
    commitIfIdle();
    if (redoStack.empty())
        return;

    undoStack.push_back (lastState);
    auto next = redoStack.back();
    redoStack.pop_back();

    proc.apvts.replaceState (next);
    lastState  = next;
    lastValues = currentValues();
}

bool LunarEQAudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    if (key.getModifiers().isCommandDown())
    {
        const int c = key.getKeyCode();
        if (c == 'z' || c == 'Z') { undo(); return true; }
        if (c == 'y' || c == 'Y') { redo(); return true; }
    }
    return false;
}

// ---------------------------------------------------------------- reset y presets
void LunarEQAudioProcessorEditor::setParamValue (const juce::String& id, float value)
{
    if (auto* prm = proc.apvts.getParameter (id))
        prm->setValueNotifyingHost (prm->convertTo0to1 (value));
}

void LunarEQAudioProcessorEditor::resetAll()
{
    commitIfIdle();
    for (int b = 0; b < kNumBands; ++b)
    {
        setParamValue (paramId (b, "on"), 0.0f);
        setParamValue (paramId (b, "solo"), 0.0f);
    }
    graph.setSelectedBand (-1);
    presetIndex = 0;
    presetPill.setName (factoryNames[0]);
}

void LunarEQAudioProcessorEditor::applyFactory (int index)
{
    commitIfIdle();
    for (int b = 0; b < kNumBands; ++b)
    {
        setParamValue (paramId (b, "on"), 0.0f);
        setParamValue (paramId (b, "solo"), 0.0f);
    }
    for (const auto& fb : factoryPreset (index))
    {
        setParamValue (paramId (fb.band, "type"), (float) fb.type);
        setParamValue (paramId (fb.band, "freq"), fb.freq);
        setParamValue (paramId (fb.band, "gain"), fb.gain);
        setParamValue (paramId (fb.band, "q"), fb.q);
        setParamValue (paramId (fb.band, "on"), 1.0f);
    }
    graph.setSelectedBand (-1);
}

void LunarEQAudioProcessorEditor::refreshUserPresets()
{
    userPresets = presetDir().findChildFiles (juce::File::findFiles, false, "*.xml");
}

void LunarEQAudioProcessorEditor::applyPresetIndex (int index)
{
    const int nFactory = factoryNames.size();
    const int total = nFactory + userPresets.size();
    if (total <= 0)
        return;

    presetIndex = ((index % total) + total) % total;

    if (presetIndex < nFactory)
    {
        applyFactory (presetIndex);
        presetPill.setName (factoryNames[presetIndex]);
    }
    else
    {
        const auto f = userPresets[presetIndex - nFactory];
        loadUserPreset (f);
        presetPill.setName (f.getFileNameWithoutExtension());
    }
}

void LunarEQAudioProcessorEditor::stepPreset (int delta)
{
    refreshUserPresets();
    applyPresetIndex (presetIndex + delta);
}

void LunarEQAudioProcessorEditor::showPresetMenu()
{
    refreshUserPresets();

    juce::PopupMenu m;
    for (int i = 0; i < factoryNames.size(); ++i)
        m.addItem (1 + i, factoryNames[i], true, presetIndex == i);
    m.addSeparator();
    m.addItem (10, "Guardar / exportar...");
    m.addItem (11, "Cargar / importar...");
    if (userPresets.size() > 0)
    {
        m.addSeparator();
        for (int i = 0; i < userPresets.size(); ++i)
            m.addItem (100 + i, userPresets[i].getFileNameWithoutExtension());
    }

    juce::Component::SafePointer<LunarEQAudioProcessorEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetPill),
                     [safe] (int r)
                     {
                         if (safe != nullptr && r != 0)
                             safe->handlePresetResult (r);
                     });
}

void LunarEQAudioProcessorEditor::handlePresetResult (int r)
{
    if (r >= 1 && r <= factoryNames.size())
        applyPresetIndex (r - 1);
    else if (r == 10)
        saveUserPreset();
    else if (r == 11)
        openLoadDialog();
    else if (r >= 100 && r < 100 + userPresets.size())
        applyPresetIndex (factoryNames.size() + (r - 100));
}

void LunarEQAudioProcessorEditor::saveUserPreset()
{
    commitIfIdle();
    chooser = std::make_unique<juce::FileChooser> ("Guardar preset", presetDir(), "*.xml");

    juce::Component::SafePointer<LunarEQAudioProcessorEditor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safe] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr)
                                  return;
                              auto f = fc.getResult();
                              if (f == juce::File())
                                  return;
                              f = f.withFileExtension (".xml");
                              if (auto xml = safe->proc.apvts.copyState().createXml())
                                  xml->writeTo (f);
                              safe->presetPill.setName (f.getFileNameWithoutExtension());
                          });
}

void LunarEQAudioProcessorEditor::openLoadDialog()
{
    chooser = std::make_unique<juce::FileChooser> ("Cargar preset", presetDir(), "*.xml");

    juce::Component::SafePointer<LunarEQAudioProcessorEditor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr)
                                  return;
                              auto f = fc.getResult();
                              if (f.existsAsFile())
                              {
                                  safe->loadUserPreset (f);
                                  safe->presetPill.setName (f.getFileNameWithoutExtension());
                              }
                          });
}

void LunarEQAudioProcessorEditor::loadUserPreset (const juce::File& file)
{
    commitIfIdle();
    auto xml = juce::XmlDocument::parse (file);
    if (xml != nullptr && xml->hasTagName (proc.apvts.state.getType()))
    {
        proc.apvts.replaceState (juce::ValueTree::fromXml (*xml));
        graph.setSelectedBand (-1);
    }
}

// ---------------------------------------------------------------- timer y dibujo
void LunarEQAudioProcessorEditor::timerCallback()
{
    panel.refresh();
    output.refresh();
    commitIfIdle();
}

void LunarEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace theme;

    if (bgCache.isValid())
        g.drawImageAt (bgCache, 0, 0);
    else
        g.fillAll (bgBottom());

    // Logo: luna con órbita
    const float mx = 26.0f, my = 18.0f, md = 38.0f;
    juce::ColourGradient lg (turquoise(), mx, my, lavender(), mx + md, my + md, false);
    g.setGradientFill (lg);
    g.drawEllipse (mx, my, md, md, 1.5f);
    g.setColour (bgTop());
    g.fillEllipse (mx + 7.0f, my - 5.0f, md * 0.88f, md * 0.88f);
    g.setGradientFill (lg);
    g.drawEllipse (mx + 7.0f, my - 5.0f, md * 0.88f, md * 0.88f, 0.0f);
    g.setColour (turquoise());
    g.fillEllipse (mx + md * 0.74f, my + md * 0.06f, 6.0f, 6.0f);

    g.setColour (juce::Colour (0xfff1f3fb));
    g.setFont (fonts::jost (23.0f, 0).withExtraKerningFactor (0.5f));
    g.drawText ("LUNAR EQ", 78, 16, 300, 30, juce::Justification::centredLeft, false);

    g.setColour (textDim());
    g.setFont (fonts::jost (9.0f, 1).withExtraKerningFactor (0.7f));
    g.drawText ("AETHEREON AUDIO", 80, 48, 300, 14, juce::Justification::centredLeft, false);

    g.setColour (textDim());
    g.setFont (fonts::jost (10.0f, 1).withExtraKerningFactor (0.3f));
    g.drawText ("ANALYZER", analyzerLabelArea, juce::Justification::centredRight, false);
}

void LunarEQAudioProcessorEditor::resized()
{
    const int W = getWidth(), H = getHeight();
    const int pad = 24;

    // Fondo cacheado: degradado y dos resplandores suaves
    bgCache = juce::Image (juce::Image::ARGB, juce::jmax (1, W), juce::jmax (1, H), true);
    {
        juce::Graphics g (bgCache);
        g.setGradientFill (juce::ColourGradient (theme::bgTop(), 0.0f, 0.0f, theme::bgBottom(), 0.0f, (float) H, false));
        g.fillAll();

        const float c1x = (float) W * 0.14f, c1y = (float) H * 0.42f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff7c5cff).withAlpha (0.16f), c1x, c1y,
                                                 juce::Colour (0xff7c5cff).withAlpha (0.0f), c1x + 720.0f, c1y, true));
        g.fillAll();

        const float c2x = (float) W * 0.88f, c2y = (float) H * 0.46f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2ed1c0).withAlpha (0.10f), c2x, c2y,
                                                 juce::Colour (0xff2ed1c0).withAlpha (0.0f), c2x - 680.0f, c2y, true));
        g.fillAll();
    }

    // Barra superior, de derecha a izquierda
    const int cy = 46;
    int x = W - pad;
    analyzerSeg.setBounds (x - 288, cy - 18, 288, 36);
    x -= 288 + 14;
    analyzerLabelArea = juce::Rectangle<int> (x - 90, cy - 8, 90, 16);
    x -= 90 + 20;
    resetBtn.setBounds (x - 34, cy - 17, 34, 34);
    redoBtn.setBounds (x - 70, cy - 17, 34, 34);
    undoBtn.setBounds (x - 106, cy - 17, 34, 34);
    x -= 106 + 14;
    presetPill.setBounds (x - 220, cy - 19, 220, 38);

    // Gráfico y zona inferior
    const int bottomH = 196;
    const int gap = 36, outW = 400;
    const int graphTop = 88;
    const int bottomY = H - 22 - bottomH;

    graph.setBounds (pad, graphTop, W - pad * 2, bottomY - 14 - graphTop);

    const int inner = W - pad * 2 - 12;
    const int panelW = inner - outW - gap;
    panel.setBounds (pad + 6, bottomY, panelW, bottomH);
    output.setBounds (pad + 6 + panelW + gap, bottomY, outW, bottomH);
}
