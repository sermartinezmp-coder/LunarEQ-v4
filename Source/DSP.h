#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

namespace lunar
{
constexpr int   kNumBands  = 16;
constexpr int   kMaxStages = 4;
constexpr float kMinFreq   = 20.0f;
constexpr float kMaxFreq   = 20000.0f;
constexpr float kMaxGainDb = 30.0f;

enum BandType { Bell = 0, LowShelf, HighShelf, LowCut, HighCut, Notch, BandPass, NumBandTypes };
enum StereoMode { ModeStereo = 0, ModeLeft, ModeRight, ModeMid, ModeSide, NumModes };

inline juce::StringArray typeNames()     { return { "Campana", "Low shelf", "High shelf", "Low cut", "High cut", "Notch", "Band pass" }; }
inline juce::StringArray slopeNames()    { return { "12 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct" }; }
inline juce::StringArray modeNames()     { return { "Estéreo", "Izquierda", "Derecha", "Mid", "Side" }; }
inline juce::StringArray analyzerNames() { return { "Apagado", "Pre", "Post", "Pre + Post" }; }

inline bool typeHasGain (int t) { return t == Bell || t == LowShelf || t == HighShelf; }
inline bool typeIsCut   (int t) { return t == LowCut || t == HighCut; }
inline bool typeUsesQ   (int t) { return ! typeIsCut (t); }

inline juce::String paramId (int band, const char* name)
{
    return juce::String ("b") + juce::String (band) + "_" + name;
}

struct Coeffs
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
};

struct BandSettings
{
    bool  on    = false;
    int   type  = Bell;
    float freq  = 1000.0f;
    float gain  = 0.0f;
    float q     = 1.0f;
    int   slope = 1;
    int   mode  = ModeStereo;
    bool  solo  = false;
};

struct BiquadState
{
    double z1 = 0.0, z2 = 0.0;
};

// Biquad, forma transpuesta directa II
inline float processSample (const Coeffs& c, BiquadState& s, float x)
{
    const double in  = (double) x;
    const double out = c.b0 * in + s.z1;
    s.z1 = c.b1 * in - c.a1 * out + s.z2;
    s.z2 = c.b2 * in - c.a2 * out;
    return (float) out;
}

// Coeficientes RBJ (Audio EQ Cookbook)
inline Coeffs makeBiquad (int type, double freq, double gainDb, double q, double sr)
{
    freq = juce::jlimit (5.0, sr * 0.49, freq);
    q    = juce::jmax (0.05, q);

    const double w0    = juce::MathConstants<double>::twoPi * freq / sr;
    const double cw    = std::cos (w0);
    const double sw    = std::sin (w0);
    const double alpha = sw / (2.0 * q);
    const double A     = std::pow (10.0, gainDb / 40.0);

    double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;

    switch (type)
    {
        case Bell:
            b0 = 1.0 + alpha * A;  b1 = -2.0 * cw;  b2 = 1.0 - alpha * A;
            a0 = 1.0 + alpha / A;  a1 = -2.0 * cw;  a2 = 1.0 - alpha / A;
            break;

        case LowShelf:
        {
            const double t = 2.0 * std::sqrt (A) * alpha;
            b0 = A * ((A + 1) - (A - 1) * cw + t);
            b1 = 2.0 * A * ((A - 1) - (A + 1) * cw);
            b2 = A * ((A + 1) - (A - 1) * cw - t);
            a0 = (A + 1) + (A - 1) * cw + t;
            a1 = -2.0 * ((A - 1) + (A + 1) * cw);
            a2 = (A + 1) + (A - 1) * cw - t;
            break;
        }

        case HighShelf:
        {
            const double t = 2.0 * std::sqrt (A) * alpha;
            b0 = A * ((A + 1) + (A - 1) * cw + t);
            b1 = -2.0 * A * ((A - 1) + (A + 1) * cw);
            b2 = A * ((A + 1) + (A - 1) * cw - t);
            a0 = (A + 1) - (A - 1) * cw + t;
            a1 = 2.0 * ((A - 1) - (A + 1) * cw);
            a2 = (A + 1) - (A - 1) * cw - t;
            break;
        }

        case LowCut:
            b0 = (1.0 + cw) * 0.5;  b1 = -(1.0 + cw);  b2 = (1.0 + cw) * 0.5;
            a0 = 1.0 + alpha;       a1 = -2.0 * cw;    a2 = 1.0 - alpha;
            break;

        case HighCut:
            b0 = (1.0 - cw) * 0.5;  b1 = 1.0 - cw;     b2 = (1.0 - cw) * 0.5;
            a0 = 1.0 + alpha;       a1 = -2.0 * cw;    a2 = 1.0 - alpha;
            break;

        case Notch:
            b0 = 1.0;  b1 = -2.0 * cw;  b2 = 1.0;
            a0 = 1.0 + alpha;  a1 = -2.0 * cw;  a2 = 1.0 - alpha;
            break;

        case BandPass:
            b0 = alpha;  b1 = 0.0;  b2 = -alpha;
            a0 = 1.0 + alpha;  a1 = -2.0 * cw;  a2 = 1.0 - alpha;
            break;

        default: break;
    }

    Coeffs c;
    c.b0 = b0 / a0;  c.b1 = b1 / a0;  c.b2 = b2 / a0;
    c.a1 = a1 / a0;  c.a2 = a2 / a0;
    return c;
}

// Devuelve el número de etapas biquad (cortes en Butterworth de 1 a 4 etapas)
inline int designBand (const BandSettings& s, double sr, Coeffs* out)
{
    if (typeIsCut (s.type))
    {
        const int stages = juce::jlimit (1, kMaxStages, s.slope + 1);
        const int N = stages * 2;
        for (int k = 0; k < stages; ++k)
        {
            const double qk = 1.0 / (2.0 * std::sin (juce::MathConstants<double>::pi * (2.0 * k + 1.0) / (2.0 * N)));
            out[k] = makeBiquad (s.type, s.freq, 0.0, qk, sr);
        }
        return stages;
    }

    out[0] = makeBiquad (s.type, s.freq, s.gain, s.q, sr);
    return 1;
}

// |H(e^jw)|^2
inline double magnitudeSq (const Coeffs& c, double w)
{
    const double cw = std::cos (w),  sw = std::sin (w);
    const double c2 = std::cos (2 * w), s2 = std::sin (2 * w);
    const double nr = c.b0 + c.b1 * cw + c.b2 * c2;
    const double ni = -(c.b1 * sw + c.b2 * s2);
    const double dr = 1.0 + c.a1 * cw + c.a2 * c2;
    const double di = -(c.a1 * sw + c.a2 * s2);
    return (nr * nr + ni * ni) / (dr * dr + di * di + 1.0e-30);
}

inline juce::String formatFreq (float v)
{
    if (v >= 1000.0f)
        return juce::String (v / 1000.0f, v >= 10000.0f ? 1 : 2) + "k";
    return juce::String ((int) std::round (v));
}

inline juce::String formatFreqHz (float v)
{
    if (v >= 1000.0f)
        return juce::String (v / 1000.0f, 2) + " kHz";
    return juce::String (v, 1) + " Hz";
}

inline float parseFreq (const juce::String& t)
{
    auto s = t.trim().toLowerCase();
    const float mult = s.contains ("k") ? 1000.0f : 1.0f;
    return s.getFloatValue() * mult;
}

inline juce::String formatGain (float v)
{
    return juce::String (v > 0.05f ? "+" : "") + juce::String (v, 1);
}

inline juce::String formatQ (float v) { return juce::String (v, 2); }
} // namespace lunar
