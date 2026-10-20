#pragma once
#include <JuceHeader.h>
#include "DarkFaderTheme.h"

// Shared drawing for the fader look used in both views: the Father's live
// level strip and the Child's own level fader. Keeping it in one place means
// the two always match.
namespace DarkFaderFader
{
    constexpr float capHeight = 16.0f;

    // Draws ticks, slot, the lightsaber glow up to `level` (0..1) and the
    // silver cap. `area` is the full fader column; the cap travels inside it
    // inset by half a cap height top and bottom. `alpha` dims the whole fader
    // (disabled / not live). When fully up the cap gets a cyan rim so "ON"
    // reads at a glance.
    inline void drawFader (juce::Graphics& g, juce::Rectangle<float> area, float level, float alpha, float capWidth)
    {
        level = juce::jlimit (0.0f, 1.0f, level);
        auto travel = area.reduced (0.0f, capHeight * 0.5f);
        float cx = area.getCentreX();

        // Scale ticks either side of the cap
        g.setColour (juce::Colour (0xff3a3d43).withMultipliedAlpha (alpha));
        for (int t = 0; t <= 10; ++t)
        {
            float ty = travel.getBottom() - travel.getHeight() * (float) t / 10.0f;
            float tickW = (t % 5 == 0) ? 9.0f : 5.0f;
            g.fillRect (cx - capWidth * 0.5f - tickW - 2.0f, ty - 0.5f, tickW, 1.0f);
            g.fillRect (cx + capWidth * 0.5f + 2.0f, ty - 0.5f, tickW, 1.0f);
        }

        // Slot
        juce::Rectangle<float> slot (cx - 3.0f, travel.getY(), 6.0f, travel.getHeight());
        g.setColour (juce::Colour (0xff0b0c0e).withMultipliedAlpha (alpha));
        g.fillRoundedRectangle (slot, 3.0f);
        g.setColour (juce::Colour (0xff222222).withMultipliedAlpha (alpha));
        g.drawRoundedRectangle (slot, 3.0f, 1.0f);

        float capY = travel.getBottom() - travel.getHeight() * level;

        // Glowing core from the bottom up to the cap
        if (level > 0.01f)
        {
            juce::Path lit;
            lit.addRoundedRectangle (cx - 2.0f, capY, 4.0f, travel.getBottom() - capY, 2.0f);
            juce::DropShadow glow (DarkFaderColours::saberCyan.withAlpha (0.85f * alpha), 10, {0, 0});
            glow.drawForPath (g, lit);
            g.setColour (DarkFaderColours::saberCore.withMultipliedAlpha (alpha));
            g.fillPath (lit);
        }

        // Silver cap
        juce::Rectangle<float> cap (cx - capWidth * 0.5f, capY - capHeight * 0.5f, capWidth, capHeight);
        juce::Path capPath; capPath.addRoundedRectangle (cap, 3.0f);

        bool fullyOn = level > 0.99f;
        if (fullyOn)
        {
            juce::DropShadow rim (DarkFaderColours::saberCyan.withAlpha (0.9f * alpha), 12, {0, 0});
            rim.drawForPath (g, capPath);
        }
        else
        {
            juce::DropShadow capShadow (juce::Colours::black.withAlpha (0.6f * alpha), 4, {0, 2});
            capShadow.drawForPath (g, capPath);
        }

        juce::ColourGradient capGrad (juce::Colour (0xffd3d8de).withMultipliedAlpha (alpha), cap.getX(), cap.getY(),
                                      juce::Colour (0xff7d848e).withMultipliedAlpha (alpha), cap.getX(), cap.getBottom(), false);
        capGrad.addColour (0.55, juce::Colour (0xff9aa1ab).withMultipliedAlpha (alpha));
        g.setGradientFill (capGrad);
        g.fillPath (capPath);

        if (fullyOn)
        {
            g.setColour (DarkFaderColours::saberCyan.withMultipliedAlpha (alpha));
            g.strokePath (capPath, juce::PathStrokeType (1.2f));
        }

        // Grip line - cyan when on, dark otherwise
        g.setColour ((fullyOn ? DarkFaderColours::saberCyan : juce::Colour (0xff2a2e35)).withMultipliedAlpha (alpha));
        g.fillRect (cap.getX() + capWidth * 0.12f, cap.getCentreY() - 1.0f, capWidth * 0.76f, 2.0f);
    }

    // Status lamp above a fader: lit cyan "ON", outlined cyan percentage for a
    // partial level, dark "OFF". `blank` draws an unlit lamp with a dash
    // (spare slot).
    inline void drawLamp (juce::Graphics& g, juce::Rectangle<float> lamp, float level, float alpha, bool blank = false)
    {
        level = juce::jlimit (0.0f, 1.0f, level);
        float r = lamp.getHeight() * 0.5f;
        juce::Path p; p.addRoundedRectangle (lamp, r);

        juce::String text;
        juce::Colour textColour;

        if (blank)
        {
            g.setColour (juce::Colour (0xff1a1c20).withMultipliedAlpha (alpha));
            g.fillPath (p);
            g.setColour (juce::Colour (0xff2a2e35).withMultipliedAlpha (alpha));
            g.strokePath (p, juce::PathStrokeType (1.0f));
            text = "-";
            textColour = DarkFaderColours::inkFaint;
        }
        else if (level > 0.99f)
        {
            juce::DropShadow glow (DarkFaderColours::saberCyan.withAlpha (0.9f * alpha), 12, {0, 0});
            glow.drawForPath (g, p);
            g.setColour (DarkFaderColours::saberCyan.withMultipliedAlpha (alpha));
            g.fillPath (p);
            text = "ON";
            textColour = juce::Colour (0xff04222c);
        }
        else if (level > 0.01f)
        {
            g.setColour (DarkFaderColours::saberCyan.withAlpha (0.14f * alpha));
            g.fillPath (p);
            g.setColour (DarkFaderColours::saberCyan.withMultipliedAlpha (alpha));
            g.strokePath (p, juce::PathStrokeType (1.2f));
            text = juce::String (juce::roundToInt (level * 100.0f)) + "%";
            textColour = DarkFaderColours::saberCore;
        }
        else
        {
            g.setColour (juce::Colour (0xff1a1c20).withMultipliedAlpha (alpha));
            g.fillPath (p);
            g.setColour (juce::Colour (0xff3a3d43).withMultipliedAlpha (alpha));
            g.strokePath (p, juce::PathStrokeType (1.0f));
            text = "OFF";
            textColour = DarkFaderColours::inkFaint;
        }

        g.setFont (getDarkFaderFont (lamp.getHeight() * 0.58f));
        g.setColour (textColour.withMultipliedAlpha (alpha));
        g.drawText (text, lamp.toNearestInt(), juce::Justification::centred, false);
    }
}
