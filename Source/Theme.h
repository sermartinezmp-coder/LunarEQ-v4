#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include "Fonts.h"

namespace lunar::theme
{
inline juce::Colour bgTop()       { return juce::Colour (0xff1d2037); }
inline juce::Colour bgBottom()    { return juce::Colour (0xff14151f); }
inline juce::Colour knobTrack()   { return juce::Colour (0xff403852); }
inline juce::Colour knobInner()   { return juce::Colour (0xff1f1b2a); }
inline juce::Colour text()        { return juce::Colour (0xffe8ebf8); }
inline juce::Colour textDim()     { return juce::Colour (0xff7f86a8); }
inline juce::Colour textSoft()    { return juce::Colour (0xff9aa1bf); }
inline juce::Colour label()       { return juce::Colour (0xffcdb6ea); }
inline juce::Colour logoRed()     { return juce::Colour (0xffff5262); }
inline juce::Colour turquoise()   { return juce::Colour (0xff5ee1d3); }
inline juce::Colour iceBlue()     { return juce::Colour (0xff6fa8ff); }
inline juce::Colour lavender()    { return juce::Colour (0xffb79cff); }
inline juce::Colour outline()     { return juce::Colour (0x12ffffff); }
inline juce::Colour panelFill()   { return juce::Colour (0x08ffffff); }
inline juce::Colour pillFill()    { return juce::Colour (0x0bffffff); }

inline juce::Colour bandColour (int i)
{
    static const juce::Colour c[16] = {
        juce::Colour (0xff5ee1d3), juce::Colour (0xff7ad0ff), juce::Colour (0xff98a8ff), juce::Colour (0xffb79cff),
        juce::Colour (0xffd3a8ff), juce::Colour (0xfff1b35b), juce::Colour (0xff8fe3a8), juce::Colour (0xffff8fa3),
        juce::Colour (0xff8ab6ff), juce::Colour (0xffe0b0ff), juce::Colour (0xffffd27a), juce::Colour (0xff6ee7ff),
        juce::Colour (0xffc4b5fd), juce::Colour (0xffffa6c9), juce::Colour (0xffa7f3d0), juce::Colour (0xfffda4af) };
    return c[juce::jlimit (0, 15, i)];
}

inline float textWidth (const juce::Font& f, const juce::String& s)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText (f, s, 0.0f, 0.0f);
    return ga.getBoundingBox (0, -1, true).getWidth();
}

// Cabecera de sección: ——— TÍTULO ———
inline void drawSectionHeader (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& text)
{
    auto font = fonts::jost (10.0f, 1).withExtraKerningFactor (0.34f);
    const float w  = textWidth (font, text);
    const float cx = (float) area.getCentreX();
    const float cy = (float) area.getCentreY();

    g.setColour (textDim());
    g.setFont (font);
    g.drawText (text, juce::Rectangle<float> (cx - w * 0.5f - 3.0f, cy - 8.0f, w + 6.0f, 16.0f),
                juce::Justification::centred, false);

    g.setColour (juce::Colours::white.withAlpha (0.09f));
    const float lx = (float) area.getX();
    const float lr = cx - w * 0.5f - 14.0f;
    const float rl = cx + w * 0.5f + 14.0f;
    if (lr > lx)
        g.fillRect (lx, cy - 0.5f, lr - lx, 1.0f);
    if ((float) area.getRight() > rl)
        g.fillRect (rl, cy - 0.5f, (float) area.getRight() - rl, 1.0f);
}

class LunarLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LunarLookAndFeel()
    {
        setColour (juce::ComboBox::textColourId,  text());
        setColour (juce::ComboBox::arrowColourId, label());
        setColour (juce::PopupMenu::backgroundColourId,            juce::Colour (0xff1a1c2c));
        setColour (juce::PopupMenu::textColourId,                  text());
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff4a4f86));
        setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);
        setColour (juce::TextButton::textColourOffId, textSoft());
        setColour (juce::TextButton::textColourOnId,  text());
    }

    // ---- Knob fino y mate (estilo del boceto)
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startA, float endA, juce::Slider& s) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
        const float d = juce::jmin (b.getWidth(), b.getHeight());
        const auto  c = b.getCentre();
        const float r = d * 0.5f;
        const float ra = r * 0.92f, rd = r * 0.68f;
        const float thick = juce::jmax (2.0f, d * 0.035f);

        const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
        const float zeroPos = bipolar ? (float) ((0.0 - s.getMinimum()) / (s.getMaximum() - s.getMinimum())) : 0.0f;
        const float zeroA = startA + zeroPos * (endA - startA);
        const float angle = startA + pos * (endA - startA);

        // sombra
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillEllipse (c.x - r * 0.84f, c.y - r * 0.84f, r * 1.68f, r * 1.68f);

        // aro fino de valor
        juce::Path track;
        track.addCentredArc (c.x, c.y, ra, ra, 0.0f, startA, endA, true);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.strokePath (track, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path val;
        if (bipolar)
            val.addCentredArc (c.x, c.y, ra, ra, 0.0f, juce::jmin (zeroA, angle), juce::jmax (zeroA, angle), true);
        else
            val.addCentredArc (c.x, c.y, ra, ra, 0.0f, startA, angle, true);
        g.setColour (s.findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (val, juce::PathStrokeType (thick * 1.1f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // disco mate
        juce::ColourGradient grad (juce::Colour (0xff565c8a), c.x - rd * 0.3f, c.y - rd * 0.5f,
                                   juce::Colour (0xff2f3458), c.x + rd, c.y + rd, true);
        g.setGradientFill (grad);
        g.fillEllipse (c.x - rd, c.y - rd, rd * 2.0f, rd * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawEllipse (c.x - rd, c.y - rd, rd * 2.0f, rd * 2.0f, 1.0f);

        // indicador
        const float dx = std::sin (angle), dy = -std::cos (angle);
        juce::Path ind;
        ind.startNewSubPath (c.x + dx * rd * 0.30f, c.y + dy * rd * 0.30f);
        ind.lineTo (c.x + dx * rd * 0.86f, c.y + dy * rd * 0.86f);
        g.setColour (juce::Colour (0xffeef1fb));
        g.strokePath (ind, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ---- Barra de balance
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos,
                           float, float, juce::Slider::SliderStyle, juce::Slider&) override
    {
        const float cy = (float) y + (float) h * 0.5f;
        g.setColour (juce::Colours::white.withAlpha (0.09f));
        g.fillRoundedRectangle ((float) x + 7.0f, cy - 2.0f, (float) w - 14.0f, 4.0f, 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.fillRect ((float) x + (float) w * 0.5f - 0.5f, cy - 6.0f, 1.0f, 12.0f);
        g.setColour (iceBlue().withAlpha (0.14f));
        g.fillEllipse (sliderPos - 11.0f, cy - 11.0f, 22.0f, 22.0f);
        g.setColour (juce::Colour (0xffeaf2ff));
        g.fillEllipse (sliderPos - 7.0f, cy - 7.0f, 14.0f, 14.0f);
    }

    // ---- Selector desplegable en píldora
    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        g.setColour (pillFill());
        g.fillRoundedRectangle (0.0f, 0.0f, (float) w, (float) h, (float) h * 0.5f);

        const float ax = (float) w - (float) h * 0.55f, ay = (float) h * 0.5f;
        juce::Path arrow;
        arrow.addTriangle (ax - 3.5f, ay - 1.5f, ax + 3.5f, ay - 1.5f, ax, ay + 2.5f);
        g.setColour (textDim());
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return fonts::inter (12.5f); }
    juce::Font getPopupMenuFont() override               { return fonts::inter (13.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return fonts::inter (11.5f); }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& lbl) override
    {
        lbl.setBounds (box.getHeight() / 2, 1, box.getWidth() - box.getHeight() * 14 / 10, box.getHeight() - 2);
        lbl.setFont (getComboBoxFont (box));
    }

    // ---- Botones en píldora
    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat();
        g.setColour (juce::Colours::white.withAlpha (down ? 0.12f : (over ? 0.08f : 0.045f)));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        const auto col = b.findColour (juce::ToggleButton::tickColourId);

        g.setColour (on ? col.withAlpha (0.16f) : juce::Colours::white.withAlpha (over ? 0.08f : 0.045f));
        g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        if (on)
        {
            g.setColour (col.withAlpha (0.55f));
            g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f);
        }
        g.setColour (on ? col.brighter (0.3f) : textSoft());
        g.setFont (fonts::inter (11.5f));
        g.drawText (b.getButtonText(), r, juce::Justification::centred, false);
    }
};
} // namespace lunar::theme
