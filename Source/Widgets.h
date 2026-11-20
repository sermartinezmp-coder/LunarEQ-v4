#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <memory>
#include "Theme.h"
#include "Shapes.h"
#include "PluginProcessor.h"

// Selector en píldora con la opción activa brillante
class SegmentedControl : public juce::Component
{
public:
    explicit SegmentedControl (juce::StringArray items, bool compactStyle = false)
        : labels (std::move (items)), compact (compactStyle) {}

    void setSelected (int i)
    {
        if (i != selected)
        {
            selected = i;
            repaint();
        }
    }
    int getSelected() const { return selected; }

    std::function<void (int)> onClick;

    void paint (juce::Graphics& g) override
    {
        using namespace lunar::theme;
        auto b = getLocalBounds().toFloat();
        g.setColour (pillFill());
        g.fillRoundedRectangle (b, b.getHeight() * 0.5f);

        const int n = labels.size();
        if (n == 0)
            return;
        const float seg = (b.getWidth() - 6.0f) / (float) n;
        const float size = compact ? 11.0f : 12.0f;

        for (int i = 0; i < n; ++i)
        {
            juce::Rectangle<float> r (3.0f + (float) i * seg, 3.0f, seg, b.getHeight() - 6.0f);
            if (i == selected)
            {
                g.setColour (turquoise().withAlpha (0.10f));
                g.fillRoundedRectangle (r.expanded (3.0f, 4.0f).translated (0.0f, 3.0f), r.getHeight());
                juce::ColourGradient grad (turquoise(), r.getX(), r.getY(), iceBlue(), r.getRight(), r.getBottom(), false);
                g.setGradientFill (grad);
                g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
                g.setColour (juce::Colour (0xff06222a));
                g.setFont (fonts::inter (size, true));
            }
            else
            {
                g.setColour (textSoft());
                g.setFont (fonts::inter (size));
            }
            g.drawText (labels[i], r, juce::Justification::centred, false);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int n = labels.size();
        if (n == 0)
            return;
        const float seg = ((float) getWidth() - 6.0f) / (float) n;
        const int i = juce::jlimit (0, n - 1, (int) ((e.position.x - 3.0f) / seg));
        if (onClick)
            onClick (i);
    }

private:
    juce::StringArray labels;
    bool compact;
    int selected = -1;
};

// Selector enlazado a un parámetro de tipo lista. offset=1: el valor 0 significa "ninguno".
class ChoiceSegment : public SegmentedControl
{
public:
    ChoiceSegment (juce::RangedAudioParameter& p, juce::StringArray items, int offsetValue, bool compactStyle = false)
        : SegmentedControl (std::move (items), compactStyle),
          off (offsetValue),
          att (p, [this] (float v) { setSelected ((int) std::round (v) - off); }, nullptr)
    {
        onClick = [this] (int i)
        {
            const int target = (i == getSelected() && off > 0) ? 0 : i + off;
            att.setValueAsCompleteGesture ((float) target);
        };
        att.sendInitialUpdate();
    }

private:
    int off;
    juce::ParameterAttachment att;
};

// Fila de iconos con la forma del filtro (campana, shelves, cortes, notch, band pass)
class FilterShapeRow : public juce::Component
{
public:
    explicit FilterShapeRow (LunarEQAudioProcessor& p) : proc (p) {}

    void setBand (int b)
    {
        att.reset();
        cur = -1;
        if (b >= 0)
            if (auto* prm = proc.apvts.getParameter (lunar::paramId (b, "type")))
            {
                att = std::make_unique<juce::ParameterAttachment> (*prm, [this] (float v)
                {
                    cur = (int) std::round (v);
                    repaint();
                }, nullptr);
                att->sendInitialUpdate();
            }
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using namespace lunar::theme;
        for (int i = 0; i < lunar::NumBandTypes; ++i)
        {
            auto r = cell (i);
            const bool on = (i == cur);
            g.setColour (on ? lavender().withAlpha (0.16f) : juce::Colours::white.withAlpha (0.04f));
            g.fillRoundedRectangle (r, 11.0f);
            if (on)
            {
                g.setColour (lavender().withAlpha (0.5f));
                g.drawRoundedRectangle (r.reduced (0.5f), 11.0f, 1.0f);
            }
            auto sp = lunar::shapePath (i);
            sp.applyTransform (juce::AffineTransform::translation (r.getCentreX() - 14.0f, r.getCentreY() - 10.0f));
            g.setColour (on ? juce::Colour (0xffd9ccff) : textSoft());
            g.strokePath (sp, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (att == nullptr)
            return;
        for (int i = 0; i < lunar::NumBandTypes; ++i)
            if (cell (i).contains (e.position))
            {
                att->setValueAsCompleteGesture ((float) i);
                return;
            }
    }

private:
    juce::Rectangle<float> cell (int i) const
    {
        return { (float) i * 44.0f, 0.0f, 38.0f, (float) getHeight() };
    }

    LunarEQAudioProcessor& proc;
    std::unique_ptr<juce::ParameterAttachment> att;
    int cur = -1;
};

// Selector de presets: ‹  Nombre ▾  ›
class PresetPill : public juce::Component
{
public:
    std::function<void()> onMenu, onPrev, onNext;

    void setName (const juce::String& n) { name = n; repaint(); }

    void paint (juce::Graphics& g) override
    {
        using namespace lunar::theme;
        auto b = getLocalBounds().toFloat();
        g.setColour (pillFill());
        g.fillRoundedRectangle (b, b.getHeight() * 0.5f);

        g.setColour (textSoft());
        const float cy = b.getCentreY();
        juce::Path l, r;
        l.startNewSubPath (22.0f, cy - 5.0f); l.lineTo (17.0f, cy); l.lineTo (22.0f, cy + 5.0f);
        r.startNewSubPath (b.getRight() - 22.0f, cy - 5.0f); r.lineTo (b.getRight() - 17.0f, cy); r.lineTo (b.getRight() - 22.0f, cy + 5.0f);
        g.strokePath (l, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.strokePath (r, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        auto font = fonts::inter (13.0f);
        const float tw = textWidth (font, name);
        const float cx = b.getCentreX();
        g.setColour (text());
        g.setFont (font);
        g.drawText (name, juce::Rectangle<float> (cx - tw * 0.5f - 6.0f, 0.0f, tw + 12.0f, b.getHeight()),
                    juce::Justification::centred, false);

        juce::Path tri;
        const float tx = cx + tw * 0.5f + 10.0f;
        tri.addTriangle (tx - 3.5f, cy - 1.5f, tx + 3.5f, cy - 1.5f, tx, cy + 2.5f);
        g.setColour (textDim());
        g.fillPath (tri);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.position.x < 36.0f)                         { if (onPrev) onPrev(); }
        else if (e.position.x > (float) getWidth() - 36.0f) { if (onNext) onNext(); }
        else                                               { if (onMenu) onMenu(); }
    }

private:
    juce::String name { "Plano" };
};
