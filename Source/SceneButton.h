#pragma once
#include <JuceHeader.h>
#include "SceneManagerHub.h"

class ColorPickerButton : public juce::Button
{
public:
    ColorPickerButton(int sIdx) : juce::Button("ColorPicker"), sceneIndex(sIdx) {}
    void paintButton(juce::Graphics& g, bool, bool) override
    {
        auto color = SceneManagerHub::getInstance()->getSceneColor(sceneIndex);
        g.setColour(color);
        g.fillEllipse(getLocalBounds().toFloat().reduced(2.0f));
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.drawEllipse(getLocalBounds().toFloat().reduced(2.0f), 1.0f);
    }
    int sceneIndex;
};

class DragHandle : public juce::Component
{
public:
    DragHandle() {}
    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        juce::Path p;
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float length = 4.0f; 
        float thickness = 2.5f; 
        float arrowBase = 6.0f;
        float arrowHeight = 4.0f;
        
        // draw a bold cross
        p.addRectangle(cx - thickness/2, cy - length, thickness, length * 2);
        p.addRectangle(cx - length, cy - thickness/2, length * 2, thickness);
        
        // arrows
        p.addTriangle(cx, cy - length - arrowHeight, cx - arrowBase/2, cy - length, cx + arrowBase/2, cy - length);
        p.addTriangle(cx, cy + length + arrowHeight, cx - arrowBase/2, cy + length, cx + arrowBase/2, cy + length);
        p.addTriangle(cx - length - arrowHeight, cy, cx - length, cy - arrowBase/2, cx - length, cy + arrowBase/2);
        p.addTriangle(cx + length + arrowHeight, cy, cx + length, cy - arrowBase/2, cx + length, cy + arrowBase/2);

        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.fillPath(p);
    }
    
    void mouseDown(const juce::MouseEvent&) override {} // Ensure we intercept mouse clicks
    
    void mouseDrag(const juce::MouseEvent& e) override {
        if (auto* dnd = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
            if (!dnd->isDragAndDropActive()) {
                auto* parent = getParentComponent();
                juce::Image dragImage(juce::Image::ARGB, parent->getWidth(), parent->getHeight(), true);
                juce::Graphics g(dragImage);
                parent->paintEntireComponent(g, false);
                dnd->startDragging("Scene", parent, juce::ScaledImage(dragImage, 1.0));
            }
        }
    }
};

class SceneButton : public juce::Button, public juce::DragAndDropTarget
{
public:
    SceneButton(int idx) : juce::Button("SceneButton"), sceneIndex(idx), colorBtn(idx)
    {
        setClickingTogglesState(true);
        setRadioGroupId(100);

        numberLabel.setJustificationType(juce::Justification::topLeft);
        numberLabel.setEditable(true, true, false);
        numberLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
        numberLabel.onTextChange = [this] {
            SceneManagerHub::getInstance()->setSceneNumber(sceneIndex, numberLabel.getText().getIntValue());
        };
        // Disable button click passing through to allow editing
        numberLabel.setInterceptsMouseClicks(true, false);
        addAndMakeVisible(numberLabel);

        nameLabel.setJustificationType(juce::Justification::centred);
        nameLabel.setEditable(true, true, false);
        nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        nameLabel.onTextChange = [this] {
            SceneManagerHub::getInstance()->setSceneName(sceneIndex, nameLabel.getText());
        };
        nameLabel.setInterceptsMouseClicks(true, false);
        addAndMakeVisible(nameLabel);
        
        colorListener.sIdx = sceneIndex;
        colorListener.parent = this;
        
        colorBtn.setInterceptsMouseClicks(true, false);
        colorBtn.onClick = [this] {
            auto* selector = new juce::ColourSelector(
                juce::ColourSelector::showColourAtTop | 
                juce::ColourSelector::showSliders | 
                juce::ColourSelector::showColourspace
            );
            selector->setName("Scene Color");
            selector->setCurrentColour(SceneManagerHub::getInstance()->getSceneColor(sceneIndex));
            selector->addChangeListener(&colorListener);
            selector->setSize(300, 300);
            juce::CallOutBox::launchAsynchronously(std::unique_ptr<juce::Component>(selector), colorBtn.getScreenBounds(), nullptr);
        };
        addAndMakeVisible(colorBtn);
        colorBtn.setVisible(false); // Hidden when locked
        
        addAndMakeVisible(dragHandle);
        dragHandle.setVisible(false); // Hidden when locked
        
        updateFromHub();
    }

    void updateFromHub()
    {
        auto* hub = SceneManagerHub::getInstance();
        juce::String currentName = hub->getSceneName(sceneIndex);
        if (nameLabel.getText() != currentName && !nameLabel.isBeingEdited())
            nameLabel.setText(currentName, juce::dontSendNotification);
            
        juce::String currentNum = juce::String(hub->getSceneNumber(sceneIndex));
        if (numberLabel.getText() != currentNum && !numberLabel.isBeingEdited())
            numberLabel.setText(currentNum, juce::dontSendNotification);
    }

    void setEditingEnabled(bool shouldEnable)
    {
        numberLabel.setEditable(shouldEnable, shouldEnable, false);
        nameLabel.setEditable(shouldEnable, shouldEnable, false);
        // Ensure clicks pass through to the button when not editable
        numberLabel.setInterceptsMouseClicks(shouldEnable, false);
        nameLabel.setInterceptsMouseClicks(shouldEnable, false);
        // Dim the text slightly if locked so it looks immutable
        nameLabel.setAlpha(shouldEnable ? 1.0f : 0.6f);
        numberLabel.setAlpha(shouldEnable ? 1.0f : 0.6f);
        
        colorBtn.setVisible(shouldEnable);
        dragHandle.setVisible(shouldEnable);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        numberLabel.setBounds(bounds.removeFromTop(20).removeFromLeft(30));
        colorBtn.setBounds(getWidth() - 22, 2, 20, 20);
        dragHandle.setBounds(6, getHeight() - 22, 16, 16);
        nameLabel.setBounds(getLocalBounds());
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        juce::Button::mouseDrag(e);
    }

    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override
    {
        return dragSourceDetails.description.toString() == "Scene" && dragSourceDetails.sourceComponent.get() != this;
    }

    void itemDragEnter(const SourceDetails& dragSourceDetails) override
    {
        isHoveredByDrag = true;
        repaint();
    }

    void itemDragMove(const SourceDetails& dragSourceDetails) override {}
    
    void itemDragExit(const SourceDetails& dragSourceDetails) override
    {
        isHoveredByDrag = false;
        repaint();
    }

    void itemDropped(const SourceDetails& dragSourceDetails) override
    {
        isHoveredByDrag = false;
        if (auto* sourceScene = dynamic_cast<SceneButton*>(dragSourceDetails.sourceComponent.get()))
        {
            if (sourceScene != this)
            {
                SceneManagerHub::getInstance()->moveScene(sourceScene->sceneIndex, this->sceneIndex);
                if (auto* editor = getParentComponent()) {
                    for (auto* child : editor->getChildren()) {
                        if (auto* siblingBtn = dynamic_cast<SceneButton*>(child)) {
                            siblingBtn->updateFromHub();
                        }
                    }
                    editor->repaint();
                }
            }
        }
        repaint();
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = getLocalBounds().toFloat();
        juce::Colour sceneColor = SceneManagerHub::getInstance()->getSceneColor(sceneIndex);
        
        if (getToggleState()) {
            juce::DropShadow shadow(sceneColor.withAlpha(0.9f), 15, {0, 0});
            juce::Path p;
            p.addRoundedRectangle(bounds.reduced(2.0f), 8.0f);
            shadow.drawForPath(g, p);
        }
        
        // Frosted Glass Background
        g.setColour(sceneColor.withAlpha(getToggleState() ? 0.4f : 0.15f));
        if (shouldDrawButtonAsDown) g.setColour(sceneColor.withAlpha(0.5f));
        
        g.fillRoundedRectangle(bounds, 8.0f);
        
        // Specular border
        g.setColour(sceneColor.withAlpha(0.6f));
        g.drawRoundedRectangle(bounds, 8.0f, 1.5f);
        
        if (isHoveredByDrag) {
            g.setColour(juce::Colours::yellow);
            g.drawRoundedRectangle(bounds.reduced(1.0f), 8.0f, 3.0f);
        }
        
        nameLabel.setColour(juce::Label::textColourId, getToggleState() ? juce::Colours::white : juce::Colours::white.withAlpha(0.8f));
        numberLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.4f));
    }

    int sceneIndex;
    bool isHoveredByDrag = false;
    juce::Label numberLabel;
    juce::Label nameLabel;
    ColorPickerButton colorBtn;
    DragHandle dragHandle;
    
    struct ColorListener : public juce::ChangeListener {
        int sIdx;
        juce::Component* parent;
        void changeListenerCallback(juce::ChangeBroadcaster* source) override {
            if (auto* cs = dynamic_cast<juce::ColourSelector*>(source)) {
                SceneManagerHub::getInstance()->setSceneColor(sIdx, cs->getCurrentColour());
                if (parent) {
                    parent->repaint();
                    if (auto* editor = parent->getParentComponent()) {
                        editor->repaint(); // Ensure child toggles glow updates
                    }
                }
            }
        }
    } colorListener;
};
