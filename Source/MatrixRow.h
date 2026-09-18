#pragma once
#include <JuceHeader.h>
#include "SceneManagerHub.h"

class MatrixRow : public juce::Component
{
public:
    MatrixRow()
    {
        addAndMakeVisible(slaveSelector);
        slaveSelector.onChange = [this] {
            activeSlaveId = slaveSelector.getSelectedId() - 1; // ID offset by 1
            if (activeSlaveId != -1) {
                auto* hub = SceneManagerHub::getInstance();
                targetGainSlider.setValue(hub->getTargetGain(activeSlaveId), juce::dontSendNotification);
            }
            repaint();
        };

        targetGainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        targetGainSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
        targetGainSlider.setRange(0.0, 2.0, 0.01);
        targetGainSlider.onValueChange = [this] {
            if (activeSlaveId != -1) {
                auto* hub = SceneManagerHub::getInstance();
                int currentScene = hub->activeScene.load();
                hub->setTargetGain(activeSlaveId, currentScene, targetGainSlider.getValue());
            }
        };
        addAndMakeVisible(targetGainSlider);

        deleteButton.setButtonText("-");
        deleteButton.onClick = [this] {
            if (onDelete) onDelete();
        };
        addAndMakeVisible(deleteButton);
    }

    void updateSlavesList()
    {
        auto* hub = SceneManagerHub::getInstance();
        auto activeSlaves = hub->getActiveSlaves();
        
        slaveSelector.clear();
        for (int id : activeSlaves)
        {
            slaveSelector.addItem(hub->getChildName(id) + " (" + juce::String(id) + ")", id + 1); 
        }
        
        if (activeSlaveId != -1 && slaveSelector.indexOfItemId(activeSlaveId + 1) != -1)
            slaveSelector.setSelectedId(activeSlaveId + 1, juce::dontSendNotification);
    }
    
    void updateFromHub()
    {
        if (activeSlaveId != -1)
        {
            auto* hub = SceneManagerHub::getInstance();
            targetGainSlider.setValue(hub->getTargetGain(activeSlaveId), juce::dontSendNotification);
        }
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(5);
        deleteButton.setBounds(bounds.removeFromRight(24).withSizeKeepingCentre(24, 24));
        bounds.removeFromRight(10);
        targetGainSlider.setBounds(bounds.removeFromRight(200));
        bounds.removeFromRight(10);
        
        // Leave space for color box
        bounds.removeFromLeft(25);
        slaveSelector.setBounds(bounds.withSizeKeepingCentre(bounds.getWidth(), 24));
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.fillRoundedRectangle(bounds, 6.0f);
        
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
        
        if (activeSlaveId != -1)
        {
            auto* hub = SceneManagerHub::getInstance();
            g.setColour(hub->getChildColor(activeSlaveId));
            g.fillRoundedRectangle(10.0f, (getHeight() - 15.0f) / 2.0f, 15.0f, 15.0f, 3.0f);
        }
    }

    juce::ComboBox slaveSelector;
    juce::Slider targetGainSlider;
    juce::TextButton deleteButton;
    int activeSlaveId = -1;
    std::function<void()> onDelete;
};
