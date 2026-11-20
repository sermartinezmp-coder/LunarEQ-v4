#include "GraphComponent.h"
#include "Theme.h"
#include <algorithm>
#include <cmath>

using namespace lunar;

namespace
{
// Escala del analizador (independiente de la escala de ganancia del EQ), en dBFS
constexpr float kAnaMin = -90.0f;
constexpr float kAnaMax = 0.0f;

float analyzerY (juce::Rectangle<float> p, float db)
{
    const float t = (juce::jlimit (kAnaMin, kAnaMax, db) - kAnaMin) / (kAnaMax - kAnaMin);
    return p.getBottom() - t * p.getHeight();
}

// Curva suave (Catmull-Rom) por una lista de puntos
juce::Path smoothPath (const std::vector<juce::Point<float>>& pts)
{
    juce::Path p;
    const int n = (int) pts.size();
    if (n < 2)
        return p;
    p.startNewSubPath (pts[0]);
    for (int i = 0; i < n - 1; ++i)
    {
        const auto p0 = pts[(size_t) juce::jmax (0, i - 1)];
        const auto p1 = pts[(size_t) i];
        const auto p2 = pts[(size_t) (i + 1)];
        const auto p3 = pts[(size_t) juce::jmin (n - 1, i + 2)];
        p.cubicTo (p1 + (p2 - p0) / 6.0f, p2 - (p3 - p1) / 6.0f, p2);
    }
    return p;
}
}

GraphComponent::GraphComponent (LunarEQAudioProcessor& p) : proc (p), popup (p)
{
    addChildComponent (popup);
    startTimerHz (30);
}

double GraphComponent::sampleRateOr() const
{
    const double sr = proc.getSampleRate();
    return sr > 0.0 ? sr : 48000.0;
}

// ---------------------------------------------------------------- coordenadas
juce::Rectangle<float> GraphComponent::plot() const
{
    return getLocalBounds().toFloat().withTrimmedLeft (56.0f).withTrimmedRight (60.0f)
                                     .withTrimmedTop (16.0f).withTrimmedBottom (40.0f);
}

juce::Rectangle<float> GraphComponent::rangeChip() const
{
    const auto p = plot();
    return { p.getRight() - 74.0f, p.getY() + 8.0f, 66.0f, 22.0f };
}

float GraphComponent::freqToX (float f) const
{
    const auto p = plot();
    return p.getX() + p.getWidth() * std::log (f / kMinFreq) / std::log (kMaxFreq / kMinFreq);
}

float GraphComponent::xToFreq (float x) const
{
    const auto p = plot();
    const float t = (x - p.getX()) / p.getWidth();
    return kMinFreq * std::pow (kMaxFreq / kMinFreq, t);
}

float GraphComponent::dbToY (float db) const
{
    const auto p = plot();
    return p.getCentreY() - db / range * p.getHeight() * 0.5f;
}

float GraphComponent::yToDb (float y) const
{
    const auto p = plot();
    return (p.getCentreY() - y) / (p.getHeight() * 0.5f) * range;
}

juce::Point<float> GraphComponent::nodePos (int b) const
{
    const auto s = proc.getBandSettings (b);
    const auto p = plot();
    const float y = dbToY (typeHasGain (s.type) ? s.gain : 0.0f);
    return { freqToX (s.freq), juce::jlimit (p.getY(), p.getBottom(), y) };
}

int GraphComponent::hitTestNode (juce::Point<float> pt) const
{
    int best = -1;
    float bestD = 14.0f * 14.0f;
    for (int b = 0; b < kNumBands; ++b)
    {
        if (! proc.getBandSettings (b).on)
            continue;
        const float d = nodePos (b).getDistanceSquaredFrom (pt);
        if (d < bestD) { bestD = d; best = b; }
    }
    return best;
}

void GraphComponent::setSelectedBand (int b)
{
    if (b == selected)
        return;
    selected = b;
    if (onSelectedChanged)
        onSelectedChanged (b);
    repaint();
}

// ---------------------------------------------------------------- parámetros
void GraphComponent::beginGesture (const juce::String& id)
{
    if (auto* p = proc.apvts.getParameter (id)) p->beginChangeGesture();
}

void GraphComponent::endGesture (const juce::String& id)
{
    if (auto* p = proc.apvts.getParameter (id)) p->endChangeGesture();
}

void GraphComponent::setParam (const juce::String& id, float v)
{
    if (auto* p = proc.apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void GraphComponent::setParamGesture (const juce::String& id, float v)
{
    beginGesture (id);
    setParam (id, v);
    endGesture (id);
}

void GraphComponent::removeBand (int b)
{
    setParamGesture (paramId (b, "on"), 0.0f);
    setParamGesture (paramId (b, "solo"), 0.0f);
    if (selected == b)
        setSelectedBand (-1);
}

void GraphComponent::addBandAt (juce::Point<float> pos)
{
    for (int b = 0; b < kNumBands; ++b)
    {
        if (proc.getBandSettings (b).on)
            continue;

        setParamGesture (paramId (b, "type"), (float) Bell);
        setParamGesture (paramId (b, "freq"), juce::jlimit (kMinFreq, kMaxFreq, xToFreq (pos.x)));
        setParamGesture (paramId (b, "gain"), juce::jlimit (-kMaxGainDb, kMaxGainDb, yToDb (pos.y)));
        setParamGesture (paramId (b, "q"), 1.0f);
        setParamGesture (paramId (b, "slope"), 1.0f);
        setParamGesture (paramId (b, "mode"), 0.0f);
        setParamGesture (paramId (b, "on"), 1.0f);
        setSelectedBand (b);
        return;
    }
}

// ---------------------------------------------------------------- ratón
void GraphComponent::mouseDown (const juce::MouseEvent& e)
{
    if (rangeChip().contains (e.position))
    {
        static const float ranges[4] = { 6.0f, 12.0f, 18.0f, 30.0f };
        int next = 0;
        for (int i = 0; i < 4; ++i)
            if (std::abs (ranges[i] - range) < 0.1f)
                next = (i + 1) % 4;
        range = ranges[next];
        repaint();
        return;
    }

    const int hit = hitTestNode (e.position);
    if (hit < 0)
    {
        setSelectedBand (-1);
        return;
    }

    setSelectedBand (hit);

    if (e.mods.isPopupMenu())
    {
        showBandMenu (hit);
        return;
    }

    dragBand = hit;
    dragHasGain = typeHasGain (proc.getBandSettings (hit).type);
    beginGesture (paramId (hit, "freq"));
    if (dragHasGain)
        beginGesture (paramId (hit, "gain"));
}

void GraphComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (dragBand < 0)
        return;

    setParam (paramId (dragBand, "freq"), juce::jlimit (kMinFreq, kMaxFreq, xToFreq (e.position.x)));
    if (dragHasGain)
        setParam (paramId (dragBand, "gain"), juce::jlimit (-kMaxGainDb, kMaxGainDb, yToDb (e.position.y)));
}

void GraphComponent::mouseUp (const juce::MouseEvent&)
{
    if (dragBand >= 0)
    {
        endGesture (paramId (dragBand, "freq"));
        if (dragHasGain)
            endGesture (paramId (dragBand, "gain"));
    }
    dragBand = -1;
}

void GraphComponent::mouseMove (const juce::MouseEvent& e)
{
    hover = hitTestNode (e.position);
    repaint();
}

void GraphComponent::mouseExit (const juce::MouseEvent&)
{
    hover = -1;
    repaint();
}

void GraphComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int hit = hitTestNode (e.position);
    if (hit >= 0)
    {
        removeBand (hit);
        return;
    }

    if (plot().contains (e.position))
        addBandAt (e.position);
}

void GraphComponent::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int band = hitTestNode (e.position);
    if (band < 0)
        return;

    const auto s = proc.getBandSettings (band);
    if (! typeUsesQ (s.type))
        return;

    const float d = w.isReversed ? -w.deltaY : w.deltaY;
    setParamGesture (paramId (band, "q"), juce::jlimit (0.1f, 18.0f, s.q * std::exp (d * 3.0f)));
}

void GraphComponent::showBandMenu (int band)
{
    juce::PopupMenu m;
    const auto s = proc.getBandSettings (band);
    const auto names = typeNames();
    for (int i = 0; i < names.size(); ++i)
        m.addItem (i + 1, names[i], true, s.type == i);
    m.addSeparator();
    m.addItem (100, "Eliminar filtro");

    juce::Component::SafePointer<GraphComponent> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                     [safe, band] (int r)
                     {
                         if (safe == nullptr || r == 0)
                             return;
                         if (r == 100)
                             safe->removeBand (band);
                         else
                             safe->setParamGesture (paramId (band, "type"), (float) (r - 1));
                     });
}

// ---------------------------------------------------------------- espectro
void GraphComponent::timerCallback()
{
    updateSpectrum();
    layoutPopup();
    repaint();
}

void GraphComponent::pushSlide (SpecState& s, const float* src, int n) const
{
    if (n >= kFftSize)
    {
        std::copy (src + (n - kFftSize), src + n, s.slide.begin());
    }
    else
    {
        std::copy (s.slide.begin() + n, s.slide.end(), s.slide.begin());
        std::copy (src, src + n, s.slide.end() - n);
    }
}

void GraphComponent::computeSpectrum (SpecState& s)
{
    std::copy (s.slide.begin(), s.slide.end(), s.data.begin());
    std::fill (s.data.begin() + kFftSize, s.data.end(), 0.0f);
    window.multiplyWithWindowingTable (s.data.data(), (size_t) kFftSize);
    fft.performFrequencyOnlyForwardTransform (s.data.data());

    const float sr = (float) sampleRateOr();
    for (int i = 0; i < kBins; ++i)
    {
        const float mag = s.data[(size_t) i] / ((float) kFftSize * 0.25f);
        float db = juce::Decibels::gainToDecibels (mag, -120.0f);
        const float f = juce::jmax (20.0f, (float) i * sr / (float) kFftSize);
        (void) f; // sin inclinación: escala calibrada en dBFS

        float& v = s.db[(size_t) i];
        v += (db - v) * (db > v ? 0.6f : 0.15f);
    }
}

void GraphComponent::updateSpectrum()
{
    auto& fifo = proc.analyzerFifo;
    const int ready = juce::jmin (fifo.getNumReady(), kTmp);

    if (ready > 0)
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead (ready, s1, n1, s2, n2);

        for (int i = 0; i < n1; ++i)
        {
            tmpPre[(size_t) i]  = proc.preBuffer[(size_t) (s1 + i)];
            tmpPost[(size_t) i] = proc.postBuffer[(size_t) (s1 + i)];
        }
        for (int i = 0; i < n2; ++i)
        {
            tmpPre[(size_t) (n1 + i)]  = proc.preBuffer[(size_t) (s2 + i)];
            tmpPost[(size_t) (n1 + i)] = proc.postBuffer[(size_t) (s2 + i)];
        }

        fifo.finishedRead (ready);
        pushSlide (preS,  tmpPre.data(),  ready);
        pushSlide (postS, tmpPost.data(), ready);
    }

    computeSpectrum (preS);
    computeSpectrum (postS);
}

// ---------------------------------------------------------------- dibujo
void GraphComponent::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto p = plot();
    const double sr = sampleRateOr();
    const int mode = proc.getAnalyzerMode();

    g.setColour (theme::panelFill());
    g.fillRoundedRectangle (bounds, 20.0f);
    g.setColour (theme::outline());
    g.drawRoundedRectangle (bounds.reduced (0.5f), 20.0f, 1.0f);

    drawGrid (g, p);

    g.saveState();
    g.reduceClipRegion (p.toNearestInt());
    if (mode == 1)
        drawSpectrum (g, p, preS, true, false);
    else if (mode == 2)
        drawSpectrum (g, p, postS, true, false);
    else if (mode == 3)
    {
        drawSpectrum (g, p, preS, true, false);
        drawSpectrum (g, p, postS, false, true);
    }
    drawCurves (g, p, sr);
    g.restoreState();

    // Selector de rango de la escala de ganancia
    const auto chip = rangeChip();
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRoundedRectangle (chip, 11.0f);
    g.setColour (theme::textSoft());
    g.setFont (fonts::inter (11.0f));
    g.drawText (juce::String::fromUTF8 ("\xC2\xB1") + juce::String ((int) range) + " dB", chip,
                juce::Justification::centred, false);

    drawNodes (g);
}

void GraphComponent::drawGrid (juce::Graphics& g, juce::Rectangle<float> p) const
{
    const float decades[] = { 10.0f, 100.0f, 1000.0f, 10000.0f };
    for (float d : decades)
        for (int m = 1; m <= 9; ++m)
        {
            const float f = d * (float) m;
            if (f < kMinFreq || f > kMaxFreq)
                continue;
            g.setColour (juce::Colours::white.withAlpha (m == 1 ? 0.07f : 0.025f));
            g.drawVerticalLine ((int) freqToX (f), p.getY(), p.getBottom());
        }

    struct Lbl { float f; const char* t; };
    const Lbl labels[] = { { 20, "20" }, { 30, "30" }, { 50, "50" }, { 100, "100" }, { 200, "200" },
                           { 300, "300" }, { 500, "500" }, { 1000, "1k" }, { 2000, "2k" }, { 3000, "3k" },
                           { 5000, "5k" }, { 10000, "10k" }, { 20000, "20k" } };
    g.setFont (fonts::inter (11.0f));
    g.setColour (theme::textDim());
    for (auto& l : labels)
    {
        const float x = freqToX (l.f);
        g.drawText (l.t, juce::Rectangle<float> (x - 20.0f, p.getBottom() + 8.0f, 40.0f, 14.0f),
                    juce::Justification::centred, false);
    }

    // Escala de ganancia del EQ (derecha)
    const float marks[] = { -range, -range * 0.5f, 0.0f, range * 0.5f, range };
    for (float db : marks)
    {
        const float y = dbToY (db);
        g.setColour (juce::Colours::white.withAlpha (db == 0.0f ? 0.14f : 0.04f));
        g.drawHorizontalLine ((int) y, p.getX(), p.getRight());
        g.setColour (theme::textDim());
        const int v = juce::roundToInt (db);
        g.drawText ((v > 0 ? "+" : "") + juce::String (v),
                    juce::Rectangle<float> (p.getRight() + 8.0f, y - 7.0f, 44.0f, 14.0f),
                    juce::Justification::centredLeft, false);
    }

    // Escala del analizador (izquierda), en dBFS
    g.setFont (fonts::inter (10.0f));
    g.setColour (juce::Colour (0xff5f6688));
    for (int db = -80; db <= 0; db += 20)
        g.drawText (juce::String (db),
                    juce::Rectangle<float> (p.getX() - 50.0f, analyzerY (p, (float) db) - 7.0f, 42.0f, 14.0f),
                    juce::Justification::centredRight, false);
}

void GraphComponent::drawSpectrum (juce::Graphics& g, juce::Rectangle<float> p, const SpecState& s,
                                   bool filled, bool thinStyle) const
{
    const float sr = (float) sampleRateOr();

    std::vector<juce::Point<float>> pts;
    for (float x = p.getX(); x <= p.getRight() + 0.5f; x += 4.0f)
    {
        const float bin = juce::jlimit (1.0f, (float) kBins - 2.0f, xToFreq (x) * (float) kFftSize / sr);
        const int i0 = (int) bin;
        const float fr = bin - (float) i0;
        const float db = s.db[(size_t) i0] + fr * (s.db[(size_t) i0 + 1] - s.db[(size_t) i0]);
        pts.emplace_back (x, analyzerY (p, db));
    }

    // suavizado espacial ligero para un aspecto limpio
    if (pts.size() > 3)
    {
        std::vector<float> ys;
        for (auto& q : pts) ys.push_back (q.y);
        for (size_t i = 1; i + 1 < pts.size(); ++i)
            pts[i].y = 0.25f * ys[i - 1] + 0.5f * ys[i] + 0.25f * ys[i + 1];
    }

    auto line = smoothPath (pts);

    if (filled)
    {
        juce::Path area (line);
        area.lineTo (p.getRight(), p.getBottom());
        area.lineTo (p.getX(), p.getBottom());
        area.closeSubPath();

        juce::ColourGradient grad (juce::Colour (0xff6fb7ff).withAlpha (0.55f), 0.0f, p.getY(),
                                   juce::Colour (0xff3a3f9c).withAlpha (0.0f), 0.0f, p.getBottom(), false);
        grad.addColour (0.55, juce::Colour (0xff5a6fe0).withAlpha (0.22f));
        g.setGradientFill (grad);
        g.fillPath (area);

        juce::ColourGradient lg (theme::turquoise().withAlpha (0.9f), p.getX(), 0.0f,
                                 theme::lavender().withAlpha (0.9f), p.getRight(), 0.0f, false);
        lg.addColour (0.6, juce::Colour (0xff7fb4ff).withAlpha (0.9f));
        g.setGradientFill (lg);
        g.strokePath (line, juce::PathStrokeType (1.6f));
    }
    else
    {
        g.setColour (juce::Colour (0xffe8e3ff).withAlpha (thinStyle ? 0.55f : 0.9f));
        g.strokePath (line, juce::PathStrokeType (1.2f));
    }
}

void GraphComponent::drawCurves (juce::Graphics& g, juce::Rectangle<float> p, double sr) const
{
    const int nPts = juce::jmax (2, (int) (p.getWidth() / 3.0f) + 1);
    const float step = p.getWidth() / (float) (nPts - 1);
    const float y0 = dbToY (0.0f);

    std::vector<float> master ((size_t) nPts, 0.0f);
    std::vector<float> bandDb ((size_t) nPts, 0.0f);

    for (int b = 0; b < kNumBands; ++b)
    {
        const auto s = proc.getBandSettings (b);
        if (! s.on)
            continue;

        Coeffs cs[kMaxStages];
        const int n = designBand (s, sr, cs);

        for (int i = 0; i < nPts; ++i)
        {
            const double f = (double) xToFreq (p.getX() + step * (float) i);
            const double w = juce::MathConstants<double>::twoPi * f / sr;
            double mag = 1.0;
            for (int st = 0; st < n; ++st)
                mag *= magnitudeSq (cs[st], w);
            const float db = (float) (10.0 * std::log10 (mag + 1.0e-20));
            bandDb[(size_t) i] = db;
            master[(size_t) i] += db;
        }

        juce::Path bp;
        bp.startNewSubPath (p.getX(), y0);
        for (int i = 0; i < nPts; ++i)
            bp.lineTo (p.getX() + step * (float) i, dbToY (juce::jlimit (-kMaxGainDb, kMaxGainDb, bandDb[(size_t) i])));
        bp.lineTo (p.getRight(), y0);
        bp.closeSubPath();

        g.setColour (theme::bandColour (b).withAlpha (b == selected ? 0.16f : 0.05f));
        g.fillPath (bp);
    }

    std::vector<juce::Point<float>> pts;
    for (int i = 0; i < nPts; ++i)
        pts.emplace_back (p.getX() + step * (float) i,
                          dbToY (juce::jlimit (-kMaxGainDb - 5.0f, kMaxGainDb + 5.0f, master[(size_t) i])));
    auto mp = smoothPath (pts);

    g.setColour (juce::Colour (0xffe9f4ff).withAlpha (0.14f));
    g.strokePath (mp, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (juce::Colour (0xfff3f8ff));
    g.strokePath (mp, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void GraphComponent::drawNodes (juce::Graphics& g) const
{
    const auto p = plot();

    for (int b = 0; b < kNumBands; ++b)
    {
        const auto s = proc.getBandSettings (b);
        if (! s.on)
            continue;

        const auto c = nodePos (b);
        const bool sel = (b == selected);
        const bool hot = (b == hover || b == dragBand);
        const auto col = theme::bandColour (b);

        if (sel)
        {
            // guía vertical punteada
            juce::Path guide, dashed;
            guide.startNewSubPath (c.x, p.getY());
            guide.lineTo (c.x, p.getBottom());
            const float dashes[2] = { 2.0f, 6.0f };
            juce::PathStrokeType (1.0f).createDashedStroke (dashed, guide, dashes, 2);
            g.setColour (col.withAlpha (0.35f));
            g.fillPath (dashed);

            g.setColour (col.withAlpha (0.16f));
            g.fillEllipse (c.x - 17.0f, c.y - 17.0f, 34.0f, 34.0f);
            g.setColour (col);
            g.fillEllipse (c.x - 8.0f, c.y - 8.0f, 16.0f, 16.0f);
            g.setColour (juce::Colours::white);
            g.drawEllipse (c.x - 8.0f, c.y - 8.0f, 16.0f, 16.0f, 1.8f);
        }
        else
        {
            const float r = hot ? 8.0f : 6.5f;
            g.setColour (col);
            g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
            g.setColour (juce::Colour (0xff14151f));
            g.drawEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f, 2.0f);
        }

        // Letra de modo (L, R, M, S) junto al nodo cuando no está en Estéreo
        if (s.mode != ModeStereo)
        {
            const char* letters[] = { "", "L", "R", "M", "S" };
            g.setColour (col);
            g.setFont (fonts::jost (10.0f, 2));
            g.drawText (letters[s.mode], juce::Rectangle<float> (c.x + 10.0f, c.y - 22.0f, 14.0f, 12.0f),
                        juce::Justification::centredLeft, false);
        }
    }

    // Información del nodo bajo el cursor o en arrastre (si no hay uno seleccionado con recuadro)
    const int labelBand = (dragBand >= 0) ? dragBand : (selected < 0 ? hover : -1);
    if (labelBand >= 0)
    {
        const auto s = proc.getBandSettings (labelBand);
        if (s.on)
        {
            juce::String t = formatFreqHz (s.freq);
            if (typeHasGain (s.type))
                t << "   " << formatGain (s.gain) << " dB";

            const auto c = nodePos (labelBand);
            auto font = fonts::inter (12.0f, true);
            const float w = theme::textWidth (font, t) + 28.0f;
            float x = c.x + 18.0f;
            if (x + w > (float) getWidth() - 4.0f)
                x = c.x - 18.0f - w;
            const float y = juce::jmax (4.0f, c.y - 40.0f);

            juce::Rectangle<float> box (x, y, w, 26.0f);
            g.setColour (juce::Colour (0xf0171a2a));
            g.fillRoundedRectangle (box, 10.0f);
            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.drawRoundedRectangle (box, 10.0f, 1.0f);
            g.setColour (theme::text());
            g.setFont (font);
            g.drawText (t, box, juce::Justification::centred, false);
        }
    }
}

// Recuadro del nodo seleccionado: aparece junto al nodo
void GraphComponent::layoutPopup()
{
    if (selected < 0 || ! proc.getBandSettings (selected).on)
    {
        popup.setVisible (false);
        return;
    }

    if (popup.getBand() != selected)
        popup.setBand (selected);
    popup.refreshInfo();

    const auto c = nodePos (selected);
    const float pw = (float) popup.getWidth(), ph = (float) popup.getHeight();
    float x = c.x + 20.0f;
    float y = c.y - ph - 24.0f;
    if (x + pw > (float) getWidth() - 6.0f)
        x = c.x - pw - 20.0f;
    if (y < 4.0f)
        y = c.y + 18.0f;

    popup.setTopLeftPosition ((int) x, (int) y);
    popup.setVisible (true);
}
