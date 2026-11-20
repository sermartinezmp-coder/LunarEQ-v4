#pragma once
#include <juce_graphics/juce_graphics.h>

namespace lunar
{
// Iconos de forma de filtro dibujados en una caja de 28 x 20
// Orden = BandType: Bell, LowShelf, HighShelf, LowCut, HighCut, Notch, BandPass
inline juce::Path shapePath (int type)
{
    juce::Path p;
    switch (type)
    {
        case 0: p.startNewSubPath (2, 14); p.cubicTo (8, 14, 9, 5, 14, 5); p.cubicTo (19, 5, 20, 14, 26, 14); break;
        case 1: p.startNewSubPath (2, 6);  p.cubicTo (10, 6, 12, 13, 18, 13); p.lineTo (26, 13); break;
        case 2: p.startNewSubPath (2, 13); p.cubicTo (8, 13, 10, 13, 14, 11); p.cubicTo (18, 9, 20, 6, 26, 6); break;
        case 3: p.startNewSubPath (2, 17); p.cubicTo (6, 17, 9, 6, 14, 6); p.lineTo (26, 6); break;
        case 4: p.startNewSubPath (2, 6);  p.lineTo (14, 6); p.cubicTo (19, 6, 22, 17, 26, 17); break;
        case 5: p.startNewSubPath (2, 7);  p.lineTo (10, 7); p.cubicTo (12, 7, 12, 16, 14, 16);
                p.cubicTo (16, 16, 16, 7, 18, 7); p.lineTo (26, 7); break;
        default: p.startNewSubPath (2, 16); p.cubicTo (8, 16, 10, 5, 14, 5); p.cubicTo (18, 5, 20, 16, 26, 16); break;
    }
    return p;
}
} // namespace lunar
