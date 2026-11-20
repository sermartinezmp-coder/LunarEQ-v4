#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"
#include "Shapes.h"

// Botón pequeño con icono dibujado
class IconButton : public juce::Button
{
public:
    enum Kind { Undo, Redo, Reset, Power, Headphones, Shape };

    explicit IconButton (Kind k) : juce::Button (juce::String()), kind (k)
    {
        setClickingTogglesState (k == Power || k == Headphones);
    }

    void setShape (int t) { if (shape != t) { shape = t; repaint(); } }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        using namespace lunar::theme;
        auto full = getLocalBounds().toFloat();
        auto b = full.withSizeKeepingCentre (juce::jmin (full.getWidth(), full.getHeight()) * 0.42f,
                                             juce::jmin (full.getWidth(), full.getHeight()) * 0.42f);
        const float cx = full.getCentreX(), cy = full.getCentreY();
        const float rad = b.getWidth() * 0.5f;
        const float pi = juce::MathConstants<float>::pi;

        if (over || down)
        {
            g.setColour (juce::Colours::white.withAlpha (down ? 0.10f : 0.06f));
            g.fillRoundedRectangle (full, full.getHeight() * 0.5f);
        }

        g.setColour (getToggleState() ? turquoise() : (down ? text() : (over ? label() : textSoft())));

        juce::Path p;
        switch (kind)
        {
            case Undo:
                p.startNewSubPath (b.getRight(), b.getY());
                p.lineTo (b.getX(), cy);
                p.lineTo (b.getRight(), b.getBottom());
                p.closeSubPath();
                g.fillPath (p);
                break;

            case Redo:
                p.startNewSubPath (b.getX(), b.getY());
                p.lineTo (b.getRight(), cy);
                p.lineTo (b.getX(), b.getBottom());
                p.closeSubPath();
                g.fillPath (p);
                break;

            case Reset:
                p.addCentredArc (cx, cy, rad, rad, 0.0f, 0.15f * pi, 1.85f * pi, true);
                g.strokePath (p, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                {
                    juce::Path head;
                    const float hx = cx + rad * std::cos (0.15f * pi), hy = cy + rad * std::sin (0.15f * pi);
                    head.addTriangle (hx - 3.0f, hy - 3.5f, hx + 3.5f, hy - 2.0f, hx + 0.5f, hy + 3.0f);
                    g.fillPath (head);
                }
                break;

            case Power:
                p.addCentredArc (cx, cy + 0.5f, rad, rad, 0.0f, 0.3f * pi, 1.7f * pi, true);
                g.strokePath (p, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.drawLine (cx, cy - rad - 1.0f, cx, cy, 1.5f);
                break;

            case Headphones:
                p.addCentredArc (cx, cy + 1.0f, rad, rad, 0.0f, -0.5f * pi, 0.5f * pi, true);
                p.addCentredArc (cx, cy + 1.0f, rad, rad, 0.0f, 1.5f * pi, 2.5f * pi, true);
                g.strokePath (p, juce::PathStrokeType (1.5f));
                g.fillRoundedRectangle (cx - rad - 1.5f, cy + 1.0f, 3.5f, rad * 0.95f, 1.2f);
                g.fillRoundedRectangle (cx + rad - 2.0f, cy + 1.0f, 3.5f, rad * 0.95f, 1.2f);
                break;

            case Shape:
            {
                auto sp = lunar::shapePath (shape);
                sp.applyTransform (juce::AffineTransform::translation (cx - 14.0f, cy - 10.0f));
                g.strokePath (sp, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                break;
            }
        }
    }

private:
    Kind kind;
    int shape = 0;
};
