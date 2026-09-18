#pragma once
#include <JuceHeader.h>

// Shared palette + typeface for the Father/Child redesign - abstracted Star
// Wars motifs (Vader chest-panel red/silver for Father, Jedi-belt bronze for
// Child, the lightsaber cyan reserved for the one glowing focal control)
// rather than literal costume artwork.
namespace DarkFaderColours
{
    static const juce::Colour fatherRed      { 0xffe5393c };
    static const juce::Colour fatherRedGlow  { 0xffff5457 };
    static const juce::Colour fatherSilver   { 0xffa4abb6 };
    static const juce::Colour childBronze    { 0xffcaa03c };
    static const juce::Colour childBronzeDim { 0xff8a6d2c };
    static const juce::Colour saberCyan      { 0xff57d9ff };
    static const juce::Colour saberCore      { 0xffeafcff };
    static const juce::Colour panelFather    { 0xff17191d };
    static const juce::Colour panelFather2   { 0xff1e2126 };
    static const juce::Colour edgeFather     { 0xff2a2e35 };
    static const juce::Colour panelChild     { 0xff1c1712 };
    static const juce::Colour panelChild2    { 0xff241d15 };
    static const juce::Colour edgeChild      { 0xff3a2f20 };
    static const juce::Colour ink            { 0xfff1f3f5 };
    static const juce::Colour inkDim         { 0xff9aa0a8 };
    static const juce::Colour inkFaint       { 0xff5d636b };
}

// The plugin bundles Montserrat-Bold.ttf as an asset but large parts of the
// original UI referenced "Myriad Pro" by name instead - a commercial font
// that's essentially never actually installed, so those draw calls were
// silently falling back to a generic system font. This gives every call
// site a one-line way to get the real bundled typeface.
inline juce::Typeface::Ptr getDarkFaderTypeface()
{
    static juce::Typeface::Ptr typeface = juce::Typeface::createSystemTypefaceFor (
        BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize);
    return typeface;
}

inline juce::Font getDarkFaderFont (float height, bool bold = true)
{
    juce::ignoreUnused (bold); // Montserrat-Bold is the only weight bundled
    return juce::Font (getDarkFaderTypeface()).withHeight (height);
}
