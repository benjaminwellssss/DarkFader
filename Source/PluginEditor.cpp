#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SceneManagerHub.h"

DarkFaderAudioProcessorEditor::DarkFaderAudioProcessorEditor (DarkFaderAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLookAndFeel);

    // Mode Toggle
    modeToggleButton.onClick = [this] { 
        if (!audioProcessor.isChildLocked) {
            audioProcessor.isFather = !audioProcessor.isFather;
            updateLayout(); 
        }
    };
    addAndMakeVisible(modeToggleButton);

    // Dynamic Scenes Buttons
    addSceneButton.onClick = [this] {
        auto* hub = SceneManagerHub::getInstance();
        int active = hub->numActiveScenes.load();
        if (!hub->isLocked.load() && active < MAX_SCENES) {
            hub->resetScene(active);
            hub->numActiveScenes.store(active + 1);
            updateLayout();
        }
    };
    addChildComponent(addSceneButton);

    removeSceneButton.onClick = [this] {
        auto* hub = SceneManagerHub::getInstance();
        int active = hub->numActiveScenes.load();
        if (!hub->isLocked.load() && active > 1) {
            hub->numActiveScenes.store(active - 1);
            if (hub->activeScene.load() >= active - 1)
                hub->activeScene.store(active - 2);
            updateLayout();
        }
    };
    addChildComponent(removeSceneButton);

    lockScenesButton.setClickingTogglesState(true);
    lockScenesButton.onClick = [this] {
        auto* hub = SceneManagerHub::getInstance();
        hub->isLocked.store(lockScenesButton.getToggleState());
        updateLayout();
    };
    addChildComponent(lockScenesButton);

    // Father UI Setup
    killSwitch.setClickingTogglesState(true);
    killSwitch.onClick = [] {
        auto* hub = SceneManagerHub::getInstance();
        hub->globalKillSwitch.store(!hub->globalKillSwitch.load());
    };
    addChildComponent(killSwitch);

    expandToggle.setClickingTogglesState(true);
    expandToggle.onClick = [this] { updateLayout(); };
    expandToggle.setVisible(false); // We can hide expandToggle entirely now or repurpose it
    addChildComponent(expandToggle);
    
    childrenRowLabel.setText("CHILDREN", juce::dontSendNotification);
    childrenRowLabel.setJustificationType(juce::Justification::centredRight);
    childrenRowLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    childrenRowLabel.setFont(juce::Font("Myriad Pro", 12.0f, juce::Font::bold));
    addChildComponent(childrenRowLabel);
    
    // Presets
    saveButton.onClick = [this] {
        fileChooser = std::make_unique<juce::FileChooser>("Save Preset",
                                                          juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
                                                          "*.puppet");
        auto folderChooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync(folderChooserFlags, [this] (const juce::FileChooser& fc) {
            juce::File file = fc.getResult();
            if (file != juce::File{}) {
                juce::MemoryBlock mb;
                audioProcessor.getStateInformation(mb);
                file.replaceWithData(mb.getData(), mb.getSize());
            }
        });
    };
    addChildComponent(saveButton);
    
    loadButton.onClick = [this] {
        fileChooser = std::make_unique<juce::FileChooser>("Load Preset",
                                                          juce::File::getSpecialLocation(juce::File::userDesktopDirectory),
                                                          "*.puppet");
        auto folderChooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser->launchAsync(folderChooserFlags, [this] (const juce::FileChooser& fc) {
            juce::File file = fc.getResult();
            if (file != juce::File{} && file.existsAsFile()) {
                juce::MemoryBlock mb;
                file.loadFileAsData(mb);
                audioProcessor.setStateInformation(mb.getData(), (int)mb.getSize());
                updateLayout();
            }
        });
    };
    addChildComponent(loadButton);

    recallDefaultButton.onClick = [this] {
        auto* hub = SceneManagerHub::getInstance();
        hub->resetScene(hub->activeScene.load());
        updateLayout();
    };
    addChildComponent(recallDefaultButton);

    fadeTimeSlider.setSliderStyle(juce::Slider::LinearBar);
    fadeTimeSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    fadeTimeSlider.setRange(0.0, 10.0, 0.1);
    fadeTimeSlider.setValue(SceneManagerHub::getInstance()->globalFadeTime.load(), juce::dontSendNotification);
    fadeTimeSlider.setTextValueSuffix(" s");
    fadeTimeSlider.onValueChange = [this] {
        SceneManagerHub::getInstance()->globalFadeTime.store(static_cast<float>(fadeTimeSlider.getValue()));
    };
    addChildComponent(fadeTimeSlider);

    fadeTimeLabel.setText("FADE TIME", juce::dontSendNotification);
    fadeTimeLabel.setJustificationType(juce::Justification::centredRight);
    addChildComponent(fadeTimeLabel);
    
    for (int i = 0; i < MAX_SCENES; ++i)
    {
        sceneButtons[i] = std::make_unique<SceneButton>(i);
        sceneButtons[i]->onClick = [i] {
            SceneManagerHub::getInstance()->activeScene.store(i);
        };
        addChildComponent(sceneButtons[i].get());
        
        for (int s = 0; s < 10; ++s)
        {
            childGridButtons[i][s] = std::make_unique<ChildToggleButton>(s, i);
            auto* btn = childGridButtons[i][s].get();
            btn->onClick = [btn, s, i] {
                int currentState = btn->getAssignmentState();
                int nextState = (currentState + 1) % 3;
                float target = 0.0f;
                if (nextState == 1) target = 1.0f;
                else if (nextState == 2) target = -1.0f;
                SceneManagerHub::getInstance()->setTargetGain(s, i, target);
                btn->repaint();
            };
            addChildComponent(btn);
        }
    }
    sceneButtons[0]->setToggleState(true, juce::dontSendNotification);

    // Child UI Setup
    gainKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gainKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    gainKnob.setRange(0.0, 1.0, 0.01);
    gainKnob.setValue(audioProcessor.gainMultiplier.getCurrentValue(), juce::dontSendNotification);
    gainKnob.textFromValueFunction = [](double value) {
        if (value < 0.01) return juce::String("OFF");
        if (value > 0.99) return juce::String("ON");
        return juce::String(value * 100.0, 0) + "%";
    };
    gainKnob.onValueChange = [this] {
        if (!audioProcessor.isFather && audioProcessor.childId != -1)
        {
            auto* hub = SceneManagerHub::getInstance();
            int currentScene = hub->activeScene.load();
            hub->setTargetGain(audioProcessor.childId, currentScene, gainKnob.getValue());
        }
    };
    addChildComponent(gainKnob);

    childNameLabel.setText(audioProcessor.customName, juce::dontSendNotification);
    childNameLabel.setJustificationType(juce::Justification::centred);
    addChildComponent(childNameLabel);

    for (int i = 1; i <= 10; ++i)
        childIdSelector.addItem("Child " + juce::String(i), i);
        
    childIdSelector.setTextWhenNothingSelected("UNASSIGNED");
    childIdSelector.setSelectedId(audioProcessor.childId + 1, juce::dontSendNotification);
    childIdSelector.onChange = [this] {
        audioProcessor.childId = childIdSelector.getSelectedId() - 1;
        updateLayout();
    };
    addChildComponent(childIdSelector);
    
    lockChildButton.setClickingTogglesState(true);
    lockChildButton.setToggleState(audioProcessor.isChildLocked, juce::dontSendNotification);
    lockChildButton.onClick = [this] {
        audioProcessor.isChildLocked = lockChildButton.getToggleState();
        updateLayout();
    };
    addChildComponent(lockChildButton);

    updateLayout();
    startTimerHz(30);
}

DarkFaderAudioProcessorEditor::~DarkFaderAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

void DarkFaderAudioProcessorEditor::timerCallback()
{
    auto* hub = SceneManagerHub::getInstance();
    if (audioProcessor.isFather)
    {
        bool isKilled = hub->globalKillSwitch.load();
        if (killSwitch.getToggleState() != isKilled)
            killSwitch.setToggleState(isKilled, juce::dontSendNotification);
            
        float currentFade = hub->globalFadeTime.load();
        if (std::abs(fadeTimeSlider.getValue() - currentFade) > 0.01)
            fadeTimeSlider.setValue(currentFade, juce::dontSendNotification);
            
        int activeScene = hub->activeScene.load();
        if (activeScene >= 0 && activeScene < MAX_SCENES && !sceneButtons[activeScene]->getToggleState())
        {
            sceneButtons[activeScene]->setToggleState(true, juce::dontSendNotification);
        }

        for (int i = 0; i < hub->numActiveScenes.load(); ++i)
        {
            sceneButtons[i]->updateFromHub();
            
            for (int s = 0; s < 10; ++s)
            {
                auto* btn = childGridButtons[i][s].get();
                float currentGain = hub->getTargetGainForScene(s, i);
                bool isOn = currentGain > 0.5f;
                if (btn->getToggleState() != isOn)
                    btn->setToggleState(isOn, juce::dontSendNotification);
            }
        }
    }
    else
    {
        float currentSmoothedGain = audioProcessor.gainMultiplier.getCurrentValue();
        if (std::abs(currentSmoothedGain - gainKnob.getValue()) > 0.001)
        {
            gainKnob.setValue(currentSmoothedGain, juce::dontSendNotification);
        }
        
        if (childNameLabel.getText() != audioProcessor.customName)
        {
            childNameLabel.setText(audioProcessor.customName, juce::dontSendNotification);
        }
    }
    repaint();
}



void DarkFaderAudioProcessorEditor::updateLayout()
{
    auto* hub = SceneManagerHub::getInstance();
    int activeScenes = hub->numActiveScenes.load();
    bool locked = hub->isLocked.load();

    lockScenesButton.setToggleState(locked, juce::dontSendNotification);
    addSceneButton.setEnabled(!locked);
    removeSceneButton.setEnabled(!locked);

    if (audioProcessor.isFather)
    {
        modeToggleButton.setButtonText("FATHER MODE");
        modeToggleButton.setEnabled(!audioProcessor.isChildLocked);
        modeToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        modeToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff333333));
        
        gainKnob.setVisible(false);
        childNameLabel.setVisible(false);
        childIdSelector.setVisible(false);
        lockChildButton.setVisible(false);

        fadeTimeSlider.setVisible(true);
        fadeTimeLabel.setVisible(true);
        
        killSwitch.setVisible(true);
        expandToggle.setVisible(true);
        childrenRowLabel.setVisible(true);
        addSceneButton.setVisible(true);
        removeSceneButton.setVisible(true);
        lockScenesButton.setVisible(true);
        saveButton.setVisible(true);
        loadButton.setVisible(true);
        recallDefaultButton.setVisible(!locked);

        for (int i = 0; i < MAX_SCENES; ++i)
        {
            bool sceneVisible = i < activeScenes;
            sceneButtons[i]->setVisible(sceneVisible);
            if (sceneVisible) {
                sceneButtons[i]->setEditingEnabled(!locked);
            }
            
            for (int s = 0; s < 10; ++s) {
                childGridButtons[i][s]->setVisible(sceneVisible);
                childGridButtons[i][s]->setEnabled(!locked);
            }
        }
        
        fadeTimeSlider.setEnabled(!locked);
        
        int numRows = (activeScenes + 4) / 5;
        setSize(std::max(1100, std::min(activeScenes, 5) * 180 + 240), std::max(480, 144 + numRows * 150)); 
    }
    else
    {
        killSwitch.setVisible(false);
        expandToggle.setVisible(false);
        addSceneButton.setVisible(false);
        removeSceneButton.setVisible(false);
        lockScenesButton.setVisible(false);
        saveButton.setVisible(false);
        loadButton.setVisible(false);
        recallDefaultButton.setVisible(false);
        childrenRowLabel.setVisible(false);
        fadeTimeSlider.setVisible(false);
        fadeTimeLabel.setVisible(false);

        for (int i = 0; i < MAX_SCENES; ++i) {
            sceneButtons[i]->setVisible(false);
            for (int s = 0; s < 10; ++s) {
                childGridButtons[i][s]->setVisible(false);
            }
        }
            
        modeToggleButton.setButtonText("CHILD MODE");
        modeToggleButton.setEnabled(!audioProcessor.isChildLocked);
        modeToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
        modeToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colours::white.withAlpha(0.8f));
            
        gainKnob.setVisible(true);
        gainKnob.setEnabled(audioProcessor.childId != -1);
        gainKnob.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
        gainKnob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

        childNameLabel.setVisible(true);
        childNameLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        childIdSelector.setVisible(true);
        childIdSelector.setEnabled(!audioProcessor.isChildLocked);
        childIdSelector.setColour(juce::ComboBox::textColourId, juce::Colours::black);
        childIdSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colours::white.withAlpha(0.8f));
        childIdSelector.setColour(juce::ComboBox::outlineColourId, juce::Colours::black);
        childIdSelector.setColour(juce::ComboBox::arrowColourId, juce::Colours::black);

        lockChildButton.setVisible(true);
        lockChildButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
        lockChildButton.setColour(juce::TextButton::buttonColourId, juce::Colours::white.withAlpha(0.8f));
        
        setSize(360, 384); // +20% size
    }
    resized();
}

void DarkFaderAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);

    if (!audioProcessor.isFather)
    {
        std::unique_ptr<juce::Drawable> bgImage = juce::Drawable::createFromImageData(BinaryData::child_bg_svg, BinaryData::child_bg_svgSize);
        if (bgImage != nullptr) {
            bgImage->drawWithin(g, getLocalBounds().toFloat(), juce::RectanglePlacement::fillDestination, 0.2f);
        } else {
            g.setColour(juce::Colour(0xffe6dbc8)); // fallback beige
            g.fillRect(getLocalBounds());
        }
        
        // High contrast background behind ID
        g.setColour(juce::Colours::black.withAlpha(0.7f));
        g.fillRoundedRectangle(10, 10, 24, 24, 4.0f);
        
        g.setColour(juce::Colours::white);
        if (auto tf = juce::Typeface::createSystemTypefaceFor(BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize)) {
            g.setFont(juce::Font(tf).withHeight(16.0f));
        } else {
            g.setFont(juce::Font("Myriad Pro", 16.0f, juce::Font::bold));
        }
        int childIdVal = audioProcessor.childId;
        juce::String badgeText = childIdVal == -1 ? juce::String("?") : juce::String(childIdVal + 1);
        g.drawText(badgeText, 10, 10, 24, 24, juce::Justification::centred);
    }
    else
    {
        std::unique_ptr<juce::Drawable> bgImage = juce::Drawable::createFromImageData(BinaryData::father_bg_svg, BinaryData::father_bg_svgSize);
        if (bgImage != nullptr) {
            bgImage->drawWithin(g, getLocalBounds().expanded(150, 0).toFloat(), juce::RectanglePlacement::stretchToFit, 0.25f);
        }
        
        // Glassy transparent blackish gradient overlay
        juce::ColourGradient glassyGrad(juce::Colours::black.withAlpha(0.8f), 0.0f, 0.0f, 
                                        juce::Colours::transparentBlack, 0.0f, (float)getHeight() * 0.8f, false);
        g.setGradientFill(glassyGrad);
        g.fillRect(getLocalBounds());
        
        // East/West mask to hide SVG edges
        juce::ColourGradient leftMask(juce::Colours::black, 0.0f, 0.0f, juce::Colours::transparentBlack, 150.0f, 0.0f, false);
        g.setGradientFill(leftMask);
        g.fillRect(0, 0, 150, getHeight());
        
        juce::ColourGradient rightMask(juce::Colours::transparentBlack, (float)getWidth() - 150.0f, 0.0f, juce::Colours::black, (float)getWidth(), 0.0f, false);
        g.setGradientFill(rightMask);
        g.fillRect(getWidth() - 150, 0, 150, getHeight());
        
        std::unique_ptr<juce::Drawable> svgLogo = juce::Drawable::createFromImageData(BinaryData::DarkFader_svg, BinaryData::DarkFader_svgSize);
        if (svgLogo != nullptr) {
            juce::Rectangle<float> logoBounds(0, 10, getWidth(), 60);
            logoBounds = logoBounds.withSizeKeepingCentre(logoBounds.getWidth() * 0.75f, logoBounds.getHeight() * 0.75f);

            svgLogo->drawWithin(g, logoBounds, juce::RectanglePlacement::centred, 1.0f);
        } else {
            // Fallback
            juce::Font logoFont(48.0f);
            if (auto tf = juce::Typeface::createSystemTypefaceFor(BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize)) {
                logoFont = juce::Font(tf).withHeight(48.0f);
            }
            juce::Path textPath;
            juce::GlyphArrangement glyphs;
            glyphs.addFittedText(logoFont, "DARKFADER", 0, 10, getWidth(), 60, juce::Justification::centred, 1);
            glyphs.createPath(textPath);
            
            g.setColour(juce::Colour(0xffff0000).withAlpha(0.6f));
            g.strokePath(textPath, juce::PathStrokeType(6.0f));
            g.setColour(juce::Colours::white);
            g.fillPath(textPath);
        }
    }

    // Version Number
    g.setColour(juce::Colours::grey.withAlpha(0.6f));
    g.setFont(juce::Font(10.0f));
    g.drawText("v1.0.3", getLocalBounds().reduced(4), juce::Justification::bottomRight, false);
}

void DarkFaderAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    if (audioProcessor.isFather) {
        // Top Left: Save/Load Presets stacked hamburger style
        saveButton.setBounds(24, 24, 168, 30);
        loadButton.setBounds(24, 60, 168, 30);
        recallDefaultButton.setBounds(24, 96, 168, 30);
        
        // Bottom Left: Mode
        modeToggleButton.setBounds(24, bounds.getHeight() - 60, 144, 42); 
        
        // Top Right: Fade Time controls (replacing zoom buttons)
        fadeTimeSlider.setBounds(bounds.getWidth() - 120, 30, 96, 24);
        fadeTimeLabel.setBounds(bounds.getWidth() - 228, 30, 96, 24);
        
        // Right Edge Column: +, -, Lock, Kill
        int rColX = bounds.getWidth() - 84;
        int rColY = 84;
        addSceneButton.setBounds(rColX, rColY, 60, 60);
        removeSceneButton.setBounds(rColX, rColY + 72, 60, 60);
        lockScenesButton.setBounds(rColX, rColY + 144, 60, 60);
        killSwitch.setBounds(rColX, rColY + 216, 60, 60);
        
        // Center: Scene Buttons and Child Grids
        int activeScenes = SceneManagerHub::getInstance()->numActiveScenes.load();
        int sceneRowH = 78; // Increased 30%
        int btnW = 172; // Increased 30%
        int maxCols = 5;
        
        int currentCols = std::min(activeScenes, maxCols);
        int totalSceneW = currentCols * btnW;
        int startX = (bounds.getWidth() - 108 - totalSceneW) / 2 + 84;
        
        childrenRowLabel.setBounds(startX - 78, 96 + sceneRowH + 12, 72, 24);
        
        for (int i = 0; i < activeScenes; ++i)
        {
            if (sceneButtons[i]->isVisible()) {
                int row = i / maxCols;
                int col = i % maxCols;
                
                int sceneX = startX + col * btnW + 2;
                int sceneY = 96 + row * 150; // Increased spacing
                
                sceneButtons[i]->setBounds(sceneX, sceneY, btnW - 5, sceneRowH);
                
                // Child Grid below the scene button
                int smallBtnW = (btnW - 5) / 5;
                int smallBtnH = 31; // Increased 30%
                int gridY = sceneY + sceneRowH + 7;
                
                for (int s = 0; s < 10; ++s) {
                    int sCol = s % 5;
                    int sRow = s / 5;
                    childGridButtons[i][s]->setBounds(sceneX + sCol * smallBtnW, gridY + sRow * smallBtnH, smallBtnW, smallBtnH);
                }
            }
        }
    }
    else
    {
        // Child UI
        modeToggleButton.setBounds(12, 12, 120, 36);
        lockChildButton.setBounds(144, 12, 36, 36);
        
        childIdSelector.setBounds(bounds.getCentreX() - 72, 60, 144, 36);
        
        gainKnob.setBounds(bounds.getCentreX() - 72, 114, 144, 144);
        childNameLabel.setBounds(0, bounds.getBottom() - 48, bounds.getWidth(), 36);
    }
}
