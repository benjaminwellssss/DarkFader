#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "DarkFaderTheme.h"
#include "SceneButton.h"
#include "IconButton.h"

class DarkFaderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DarkFaderLookAndFeel()
    {
        setColour (juce::Slider::thumbColourId, DarkFaderColours::saberCore);
        setColour (juce::Slider::rotarySliderFillColourId, DarkFaderColours::saberCyan);
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff2a2117));

        setColour (juce::TextButton::buttonColourId, DarkFaderColours::panelFather2);
        setColour (juce::TextButton::buttonOnColourId, DarkFaderColours::fatherRed);
        setColour (juce::TextButton::textColourOffId, DarkFaderColours::inkDim);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);

        setColour (juce::ComboBox::backgroundColourId, DarkFaderColours::panelChild2);
        setColour (juce::ComboBox::outlineColourId, DarkFaderColours::edgeChild);
    }

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& font) override
    {
        if (auto tf = getDarkFaderTypeface())
            return tf;
        return LookAndFeel_V4::getTypefaceForFont (font);
    }

    juce::Font getLabelFont (juce::Label&) override
    {
        return getDarkFaderFont (14.5f);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(15);

        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto lineW = 8.0f;
        auto arcRadius = radius - lineW * 0.5f;

        // Background track (dark)
        juce::Path backgroundArc;
        backgroundArc.addCentredArc (bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius,
                                     0.0f, rotaryStartAngle, rotaryEndAngle, true);

        g.setColour (juce::Colour(0xff222222));
        g.strokePath (backgroundArc, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (slider.isEnabled())
        {
            juce::Path valueArc;
            valueArc.addCentredArc (bounds.getCentreX(), bounds.getCentreY(), arcRadius, arcRadius,
                                    0.0f, rotaryStartAngle, toAngle, true);

            juce::Colour glowColor = DarkFaderColours::saberCyan; // Lightsaber - the one glowing focal control

            // Outer glow
            juce::DropShadow shadow(glowColor.withAlpha(0.85f), 12, {0, 0});
            shadow.drawForPath(g, valueArc);

            // Core
            g.setColour (DarkFaderColours::saberCore);
            g.strokePath (valueArc, juce::PathStrokeType (lineW - 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Thumb
            juce::Point<float> thumbPoint (bounds.getCentreX() + arcRadius * std::cos (toAngle - juce::MathConstants<float>::halfPi),
                                           bounds.getCentreY() + arcRadius * std::sin (toAngle - juce::MathConstants<float>::halfPi));

            juce::Path thumbP;
            thumbP.addEllipse(juce::Rectangle<float>(14.0f, 14.0f).withCentre(thumbPoint));

            // Thumb Glow
            shadow.drawForPath(g, thumbP);

            g.setColour (DarkFaderColours::saberCore);
            g.fillPath(thumbP);
        }
    }

    juce::Slider::SliderLayout getSliderLayout (juce::Slider& slider) override
    {
        juce::Slider::SliderLayout layout;
        layout.sliderBounds = slider.getLocalBounds();
        layout.textBoxBounds = slider.getLocalBounds().withSizeKeepingCentre(80, 20);
        return layout;
    }
};

class ChildToggleButton : public juce::Button
{
public:
    ChildToggleButton(int sId, int scIndex) : juce::Button("ChildToggle"), childId(sId), sceneIndex(scIndex)
    {
    }
    
    int getAssignmentState() const
    {
        float target = SceneManagerHub::getInstance()->getTargetGainForScene(childId, sceneIndex);
        if (target > 0.9f) return 1;
        if (target < -0.9f) return 2;
        return 0;
    }

    void clicked() override
    {
        Button::clicked();
    }
    
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat();

        juce::String text = juce::String(childId + 1);
        g.setFont (getDarkFaderFont (bounds.getHeight() * 0.62f));

        int currentState = getAssignmentState();

        if (currentState == 1) { // ON - father red, branded rather than per-scene colour
            juce::Colour c = DarkFaderColours::fatherRed;
            g.setColour(c.withAlpha(0.8f));
            if (shouldDrawButtonAsDown) g.setColour(c);
            g.fillRoundedRectangle(bounds, 3.0f);

            g.setColour(DarkFaderColours::fatherRedGlow);
            g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

            g.setColour(juce::Colours::white);
        } else if (currentState == 2) { // IGNORE - bronze (matches the Child identity colour)
            juce::Colour c = DarkFaderColours::childBronze;
            g.setColour(c.withAlpha(0.65f));
            if (shouldDrawButtonAsDown) g.setColour(c);
            g.fillRoundedRectangle(bounds, 3.0f);

            g.setColour(c.brighter());
            g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

            g.setColour(juce::Colours::black);
        } else { // OFF
            g.setColour(DarkFaderColours::panelFather2);
            if (shouldDrawButtonAsHighlighted) g.setColour(juce::Colour(0xff2a2d33));
            g.fillRoundedRectangle(bounds, 3.0f);

            g.setColour(juce::Colours::black.withAlpha(0.8f));
            g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

            g.setColour(DarkFaderColours::inkFaint);
        }
        g.drawText(text, bounds, juce::Justification::centred, false);
    }

    int childId;
    int sceneIndex;
};

class DarkFaderAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                          public juce::DragAndDropContainer,
                                          private juce::Timer
{
public:
    DarkFaderAudioProcessorEditor (DarkFaderAudioProcessor&);
    ~DarkFaderAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    DarkFaderAudioProcessor& audioProcessor;
    DarkFaderLookAndFeel customLookAndFeel;

    // --- Shared Components ---
    juce::TextButton modeToggleButton { "FATHER / CHILD" };
    juce::TextButton expandToggle { "EXPAND" };
    IconButton killSwitch { IconButton::Kill };
    
    std::array<std::unique_ptr<SceneButton>, MAX_SCENES> sceneButtons;

    IconButton addSceneButton { IconButton::Plus };
    IconButton removeSceneButton { IconButton::Minus };
    IconButton lockScenesButton { IconButton::Lock };
    
    juce::TextButton saveButton { "SAVE PRESET" };
    juce::TextButton loadButton { "LOAD PRESET" };
    juce::TextButton recallDefaultButton { "RECALL DEFAULT" };
    std::unique_ptr<juce::FileChooser> fileChooser;
    
    // UI Scale Buttons
    juce::Slider fadeTimeSlider;
    juce::Label fadeTimeLabel;
    
    // Grid Components
    std::array<std::array<std::unique_ptr<ChildToggleButton>, 10>, MAX_SCENES> childGridButtons;

    // --- Child Components ---
    juce::Slider gainKnob;
    juce::Label childNameLabel;
    juce::ComboBox childIdSelector;
    IconButton lockChildButton { IconButton::Lock };
    
    juce::Label childrenRowLabel;

    void updateLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkFaderAudioProcessorEditor)
};
