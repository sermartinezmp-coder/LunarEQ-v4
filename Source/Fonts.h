#pragma once
#include <juce_graphics/juce_graphics.h>

#if LUNAR_HAS_FONTS
 #include <BinaryData.h>
#endif

namespace lunar::fonts
{
struct Set
{
    juce::Typeface::Ptr jostLight, jostRegular, jostMedium, interRegular, interMedium;
};

inline Set& set()
{
    static Set s = []
    {
        Set x;
#if LUNAR_HAS_FONTS
        auto mk = [] (const void* d, int n) { return juce::Typeface::createSystemTypefaceFor (d, (size_t) n); };
        x.jostLight    = mk (BinaryData::Jost_Light_ttf,          BinaryData::Jost_Light_ttfSize);
        x.jostRegular  = mk (BinaryData::Jost_Regular_ttf,        BinaryData::Jost_Regular_ttfSize);
        x.jostMedium   = mk (BinaryData::Jost_Medium_ttf,         BinaryData::Jost_Medium_ttfSize);
        x.interRegular = mk (BinaryData::Inter_18pt_Regular_ttf,  BinaryData::Inter_18pt_Regular_ttfSize);
        x.interMedium  = mk (BinaryData::Inter_18pt_Medium_ttf,   BinaryData::Inter_18pt_Medium_ttfSize);
#endif
        return x;
    }();
    return s;
}

inline juce::Font make (const juce::Typeface::Ptr& tf, float size, int styleFlags)
{
    if (tf != nullptr)
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    return juce::Font (juce::FontOptions (size, styleFlags));
}

// weight: 0 ligera, 1 normal, 2 media
inline juce::Font jost (float size, int weight = 1)
{
    auto& s = set();
    return make (weight == 0 ? s.jostLight : (weight == 2 ? s.jostMedium : s.jostRegular),
                 size, weight == 2 ? juce::Font::bold : juce::Font::plain);
}

inline juce::Font inter (float size, bool medium = false)
{
    auto& s = set();
    return make (medium ? s.interMedium : s.interRegular, size, medium ? juce::Font::bold : juce::Font::plain);
}
} // namespace lunar::fonts
