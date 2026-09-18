#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SceneButton.h"
#include "IconButton.h"

class DarkFaderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DarkFaderLookAndFeel()
    {
        setColour (juce::Slider::thumbColourId, juce::Colour(0xffa8e6cf)); // Pastel Cyan
        setColour (juce::Slider::rotarySliderFillColourId, juce::Colour(0xffa8e6cf));
        setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff404040));
        
        setColour (juce::TextButton::buttonColourId, juce::Colour(0xff3d3d3d));
        setColour (juce::TextButton::buttonOnColourId, juce::Colour(0xffa8e6cf));
        setColour (juce::TextButton::textColourOffId, juce::Colours::white);
        setColour (juce::TextButton::textColourOnId, juce::Colour(0xff2d2d2d));
        
        setColour (juce::ComboBox::backgroundColourId, juce::Colour(0xff2d2d2d));
        setColour (juce::ComboBox::outlineColourId, juce::Colour(0xff404040));
    }
    
    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& font) override
    {
        return juce::Typeface::createSystemTypefaceFor(juce::Font("Myriad Pro", font.getHeight(), juce::Font::bold));
    }
    
    juce::Font getLabelFont (juce::Label& label) override
    {
        return juce::Font("Myriad Pro", 14.5f, juce::Font::bold);
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

            juce::Colour glowColor = juce::Colour(0xff00bfff); // Blue lightsaber
            
            // Outer glow
            juce::DropShadow shadow(glowColor.withAlpha(0.9f), 12, {0, 0});
            shadow.drawForPath(g, valueArc);
            
            // Core
            g.setColour (juce::Colours::white);
            g.strokePath (valueArc, juce::PathStrokeType (lineW - 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            
            // Thumb
            juce::Point<float> thumbPoint (bounds.getCentreX() + arcRadius * std::cos (toAngle - juce::MathConstants<float>::halfPi),
                                           bounds.getCentreY() + arcRadius * std::sin (toAngle - juce::MathConstants<float>::halfPi));

            juce::Path thumbP;
            thumbP.addEllipse(juce::Rectangle<float>(14.0f, 14.0f).withCentre(thumbPoint));
            
            // Thumb Glow
            shadow.drawForPath(g, thumbP);
            
            g.setColour (juce::Colours::white);
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
        auto* hub = SceneManagerHub::getInstance();
        
        juce::Colour glowColor = hub->getSceneColor(sceneIndex);
        juce::String text = juce::String(childId + 1);
        g.setFont(juce::Font("Myriad Pro", bounds.getHeight() * 0.7f, juce::Font::bold));
        
        int currentState = getAssignmentState();
        
        if (currentState == 1) { // ON (Color)
            g.setColour(glowColor.withAlpha(0.8f));
            if (shouldDrawButtonAsDown) g.setColour(glowColor);
            g.fillRoundedRectangle(bounds, 3.0f);
            
            g.setColour(glowColor.brighter());
            g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
            
            g.setColour(juce::Colours::white);
        } else if (currentState == 2) { // IGNORE (Yellow)
            juce::Colour yellowColor = juce::Colours::yellow;
            g.setColour(yellowColor.withAlpha(0.6f));
            if (shouldDrawButtonAsDown) g.setColour(yellowColor);
            g.fillRoundedRectangle(bounds, 3.0f);
            
            g.setColour(yellowColor.brighter());
            g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
            
            g.setColour(juce::Colours::black);
        } else { // OFF (Grey)
            g.setColour(juce::Colour(0xff404040));
            if (shouldDrawButtonAsHighlighted) g.setColour(juce::Colour(0xff555555));
            g.fillRoundedRectangle(bounds, 3.0f);
            
            g.setColour(juce::Colours::black.withAlpha(0.8f));
            g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
            
            g.setColour(juce::Colours::lightgrey);
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
