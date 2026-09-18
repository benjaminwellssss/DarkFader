#pragma once
#include <JuceHeader.h>
#include "DarkFaderTheme.h"

class IconButton : public juce::Button
{
public:
    enum IconType { Plus, Minus, Lock, Gear, Kill };

    IconButton(IconType type) : juce::Button("IconButton"), iconType(type)
    {
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat();
        bool btnIsEnabled = isEnabled();

        bool killEngaged = (iconType == Kill && getToggleState());

        // Glow when the kill switch is engaged - the one alarming state this rail has
        if (killEngaged)
        {
            juce::DropShadow shadow (DarkFaderColours::fatherRed.withAlpha (0.6f), 14, {0, 0});
            juce::Path p;
            p.addRoundedRectangle (bounds, bounds.getHeight() * 0.2f);
            shadow.drawForPath (g, p);
        }

        // Apple Liquid Glass Frosted Background
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.fillRoundedRectangle(bounds, bounds.getHeight() * 0.2f);

        g.setColour(killEngaged ? DarkFaderColours::fatherRed : juce::Colours::white.withAlpha(0.4f));
        g.drawRoundedRectangle(bounds, bounds.getHeight() * 0.2f, killEngaged ? 2.0f : 1.5f);

        // Icon Color
        juce::Colour iconCol = btnIsEnabled ? juce::Colours::white : juce::Colours::white.withAlpha(0.3f);
        if (iconType == Lock)
            iconCol = getToggleState() ? juce::Colour(0xff4fbf6a) : DarkFaderColours::fatherRed; // Locked = safe green, Unlocked = father red

        if (shouldDrawButtonAsDown) iconCol = iconCol.brighter(0.2f);

        auto iconBounds = bounds.reduced(6.0f);
        juce::Path p;
        float strokeW = iconBounds.getWidth() * 0.08f;

        if (iconType == Plus)
        {
            float cx = iconBounds.getCentreX();
            float cy = iconBounds.getCentreY();
            float r = iconBounds.getWidth() * 0.35f;
            p.addLineSegment(juce::Line<float>(cx - r, cy, cx + r, cy), strokeW);
            p.addLineSegment(juce::Line<float>(cx, cy - r, cx, cy + r), strokeW);
            
            g.setColour(iconCol);
            g.strokePath(p, juce::PathStrokeType(strokeW, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        }
        else if (iconType == Minus)
        {
            float cx = iconBounds.getCentreX();
            float cy = iconBounds.getCentreY();
            float r = iconBounds.getWidth() * 0.35f;
            p.addLineSegment(juce::Line<float>(cx - r, cy, cx + r, cy), strokeW);
            
            g.setColour(iconCol);
            g.strokePath(p, juce::PathStrokeType(strokeW, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        }
        else if (iconType == Lock)
        {
            float w = iconBounds.getWidth() * 0.6f;
            float h = iconBounds.getHeight() * 0.45f;
            float cx = iconBounds.getCentreX();
            float cy = iconBounds.getCentreY() + iconBounds.getHeight() * 0.1f;
            
            // Body
            juce::Path body;
            body.addRoundedRectangle(cx - w/2, cy - h/2, w, h, w * 0.2f);
            
            // Shackle
            juce::Path shackle;
            float shackleR = w * 0.35f;
            
            if (getToggleState()) { // Locked
                shackle.startNewSubPath(cx - shackleR, cy - h/2);
                shackle.lineTo(cx - shackleR, cy - h/2 - shackleR * 0.5f);
                shackle.addCentredArc(cx, cy - h/2 - shackleR * 0.5f, shackleR, shackleR, 0.0f, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
                shackle.lineTo(cx + shackleR, cy - h/2);
            } else { // Unlocked
                shackle.startNewSubPath(cx - shackleR, cy - h/2);
                shackle.lineTo(cx - shackleR, cy - h/2 - shackleR * 0.8f);
                shackle.addCentredArc(cx, cy - h/2 - shackleR * 0.8f, shackleR, shackleR, 0.0f, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
            }
            
            g.setColour(iconCol);
            g.fillPath(body);
            g.strokePath(shackle, juce::PathStrokeType(w * 0.2f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        }
        else if (iconType == Gear)
        {
            float cx = iconBounds.getCentreX();
            float cy = iconBounds.getCentreY();
            float r = iconBounds.getWidth() * 0.3f;
            
            p.addEllipse(cx - r, cy - r, r*2, r*2);
            p.addEllipse(cx - r*0.4f, cy - r*0.4f, r*0.8f, r*0.8f);
            
            int numTeeth = 8;
            float angleStep = juce::MathConstants<float>::twoPi / numTeeth;
            for (int i = 0; i < numTeeth; ++i) {
                float angle = i * angleStep;
                float x1 = cx + std::cos(angle) * r;
                float y1 = cy + std::sin(angle) * r;
                float x2 = cx + std::cos(angle) * (r * 1.35f);
                float y2 = cy + std::sin(angle) * (r * 1.35f);
                p.addLineSegment(juce::Line<float>(x1, y1, x2, y2), r * 0.5f);
            }
            
            g.setColour(iconCol);
            g.strokePath(p, juce::PathStrokeType(strokeW, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        }
        else if (iconType == Kill)
        {
            float cx = iconBounds.getCentreX();
            float cy = iconBounds.getCentreY();
            float r = iconBounds.getWidth() * 0.35f;
            
            p.addLineSegment(juce::Line<float>(cx - r, cy - r, cx + r, cy + r), strokeW);
            p.addLineSegment(juce::Line<float>(cx + r, cy - r, cx - r, cy + r), strokeW);
            
            iconCol = getToggleState() ? DarkFaderColours::fatherRedGlow : juce::Colours::indianred.withAlpha(0.6f);
            if (shouldDrawButtonAsDown) iconCol = iconCol.brighter();
            
            g.setColour(iconCol);
            g.strokePath(p, juce::PathStrokeType(strokeW, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        }
    }

private:
    IconType iconType;
};
