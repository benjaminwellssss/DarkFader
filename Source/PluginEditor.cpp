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
    childrenRowLabel.setFont(getDarkFaderFont(12.0f));
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
    gainKnob.setSliderStyle(juce::Slider::LinearVertical);
    gainKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    gainKnob.setSliderSnapsToMousePosition(false); // drag the cap, don't jump on click
    gainKnob.setRange(0.0, 1.0, 0.01);
    gainKnob.setValue(audioProcessor.getCurrentGain(), juce::dontSendNotification);
    gainKnob.textFromValueFunction = [](double value) {
        if (value < 0.01) return juce::String("OFF");
        if (value > 0.99) return juce::String("ON");
        return juce::String(value * 100.0, 0) + "%";
    };
    gainKnob.updateText(); // value was set before the formatter existed, so refresh the readout
    gainKnob.onValueChange = [this] {
        if (!audioProcessor.isFather && audioProcessor.childId != -1)
        {
            audioProcessor.lastManualEditMs.store (juce::Time::getMillisecondCounter());
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
        SceneManagerHub::getInstance()->setChildName (audioProcessor.childId, audioProcessor.customName);
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
        // Don't move the fader out from under the user's hand while they drag it.
        float currentSmoothedGain = audioProcessor.getCurrentGain();
        if (! gainKnob.isMouseButtonDown() && std::abs(currentSmoothedGain - gainKnob.getValue()) > 0.001)
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
        int gridBottom = 130 + (numRows - 1) * 150 + 78 + 7 + 62;
        setSize(std::max(1100, std::min(activeScenes, 5) * 180 + 240),
                std::max(514, gridBottom + 24 + faderStripHeight + 72));
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
        
        setSize(360, 448);
    }
    resized();
}

// Curved "pauldron" panel silhouette shared by both views - the shoulder-
// armor dip abstracted into panel geometry instead of literal costume art.
static void drawDarkFaderPanelBackground (juce::Graphics& g, juce::Rectangle<int> bounds,
                                           juce::Colour panelTop, juce::Colour panelBottom,
                                           juce::Colour curveFill, juce::Colour glowTint)
{
    auto b = bounds.toFloat();
    juce::ColourGradient grad (panelTop, b.getX(), b.getY(), panelBottom, b.getX(), b.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRect (b);

    float curveH = juce::jmin (110.0f, b.getHeight() * 0.22f);
    juce::Path curve;
    curve.startNewSubPath (b.getX(), b.getY());
    curve.lineTo (b.getX(), curveH);
    curve.cubicTo (b.getWidth() * 0.27f, curveH * 0.28f,
                    b.getWidth() * 0.73f, curveH * 0.28f,
                    b.getRight(), curveH);
    curve.lineTo (b.getRight(), b.getY());
    curve.closeSubPath();
    g.setColour (curveFill);
    g.fillPath (curve);
    g.setColour (curveFill.brighter (0.3f).withAlpha (0.5f));
    g.strokePath (curve, juce::PathStrokeType (1.4f));

    juce::ColourGradient glow (glowTint.withAlpha (0.28f), b.getCentreX(), 0.0f,
                                glowTint.withAlpha (0.0f), b.getCentreX(), b.getHeight() * 0.6f, true);
    g.setGradientFill (glow);
    g.fillRect (b);
}

void DarkFaderAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    if (!audioProcessor.isFather)
    {
        drawDarkFaderPanelBackground (g, getLocalBounds(),
            DarkFaderColours::panelChild, juce::Colour (0xff100d09),
            juce::Colour (0xff100d09), DarkFaderColours::childBronze);

        // ID plate - rounded mono badge instead of a plain square
        int childIdVal = audioProcessor.childId;
        juce::String idText = childIdVal == -1 ? juce::String ("UNASSIGNED") : ("CHILD " + juce::String (childIdVal + 1));
        g.setFont (getDarkFaderFont (11.5f));
        int plateW = g.getCurrentFont().getStringWidth (idText) + 20;

        // Top-right corner: the mode/lock buttons own the top-left.
        juce::Rectangle<float> plate (static_cast<float> (getWidth() - plateW - 12), 19.0f, static_cast<float> (plateW), 22.0f);
        g.setColour (juce::Colour (0xff100d09));
        g.fillRoundedRectangle (plate, 6.0f);
        g.setColour (childIdVal == -1 ? DarkFaderColours::inkFaint : DarkFaderColours::childBronze);
        g.drawRoundedRectangle (plate, 6.0f, 1.2f);
        g.setColour (childIdVal == -1 ? DarkFaderColours::inkFaint : DarkFaderColours::childBronze);
        g.drawText (idText, plate.toNearestInt(), juce::Justification::centred, false);

        // Status lamp + caption above the fader, same lamp as the Father's strip
        {
            float level = (float) gainKnob.getValue();
            juce::Rectangle<float> lamp ((float) getWidth() * 0.5f - 34.0f, 106.0f, 68.0f, 22.0f);
            DarkFaderFader::drawLamp (g, lamp, level, childIdVal == -1 ? 0.35f : 1.0f);

            auto* hubForCaption = SceneManagerHub::getInstance();
            juce::String caption = childIdVal == -1
                ? juce::String ("NO SCENES - FULL VOLUME")
                : "LEVEL IN SCENE " + juce::String (hubForCaption->getSceneNumber (hubForCaption->activeScene.load())).paddedLeft ('0', 2);
            g.setFont (getDarkFaderFont (10.0f));
            g.setColour (DarkFaderColours::inkFaint);
            g.drawText (caption, juce::Rectangle<int> (0, 132, getWidth(), 14), juce::Justification::centred, false);
        }

        // Bronze assignment-status row at the bottom - the "belt" motif
        // abstracted into a real readout instead of costume leatherwork.
        auto* hub = SceneManagerHub::getInstance();
        juce::String statusText = childIdVal == -1
            ? juce::String ("Not assigned to a slot")
            : ("Assigned - Scene " + juce::String (hub->getSceneNumber (hub->activeScene.load())));

        juce::Rectangle<float> row (14.0f, static_cast<float> (getHeight() - 34), static_cast<float> (getWidth() - 28), 24.0f);
        g.setColour (juce::Colour (0xff140f0a));
        g.fillRoundedRectangle (row, 8.0f);
        g.setColour (DarkFaderColours::edgeChild);
        g.drawRoundedRectangle (row, 8.0f, 1.0f);

        float buckleD = 14.0f;
        juce::Rectangle<float> buckle (row.getX() + 8.0f, row.getCentreY() - buckleD * 0.5f, buckleD, buckleD);
        g.setColour (juce::Colours::transparentBlack);
        g.fillEllipse (buckle);
        g.setColour (childIdVal == -1 ? DarkFaderColours::inkFaint : DarkFaderColours::childBronze);
        g.drawEllipse (buckle, 1.6f);

        g.setFont (getDarkFaderFont (11.5f));
        g.setColour (DarkFaderColours::inkDim);
        g.drawText (statusText, row.withTrimmedLeft (buckleD + 16.0f).toNearestInt(), juce::Justification::centredLeft, true);
    }
    else
    {
        drawDarkFaderPanelBackground (g, getLocalBounds(),
            DarkFaderColours::panelFather, juce::Colour (0xff101113),
            juce::Colour (0xff0d0e10), DarkFaderColours::fatherRed);

        std::unique_ptr<juce::Drawable> svgLogo = juce::Drawable::createFromImageData(BinaryData::DarkFader_svg, BinaryData::DarkFader_svgSize);
        if (svgLogo != nullptr) {
            juce::Rectangle<float> logoBounds(0, 10, getWidth(), 60);
            logoBounds = logoBounds.withSizeKeepingCentre(logoBounds.getWidth() * 0.75f, logoBounds.getHeight() * 0.75f);

            svgLogo->drawWithin(g, logoBounds, juce::RectanglePlacement::centred, 1.0f);
        } else {
            // Fallback
            juce::Font logoFont = getDarkFaderFont (48.0f);
            juce::Path textPath;
            juce::GlyphArrangement glyphs;
            glyphs.addFittedText(logoFont, "DARKFADER", 0, 10, getWidth(), 60, juce::Justification::centred, 1);
            glyphs.createPath(textPath);

            g.setColour(DarkFaderColours::fatherRed.withAlpha(0.6f));
            g.strokePath(textPath, juce::PathStrokeType(6.0f));
            g.setColour(juce::Colours::white);
            g.fillPath(textPath);
        }

        // Chest-panel-inspired status cluster - Vader's chest box abstracted
        // into a live readout (active scene, kill/lock state) instead of a
        // static costume prop.
        {
            float clusterW = 300.0f, clusterH = 32.0f;
            juce::Rectangle<float> cluster ((static_cast<float> (getWidth()) - clusterW) * 0.5f, 74.0f, clusterW, clusterH);

            g.setColour (juce::Colour (0xff0d0e10));
            g.fillRoundedRectangle (cluster, 8.0f);
            g.setColour (DarkFaderColours::fatherSilver);
            g.drawRoundedRectangle (cluster, 8.0f, 1.4f);

            auto inner = cluster.reduced (8.0f, 6.0f);

            auto pillArea = inner.removeFromLeft (46.0f);
            juce::Rectangle<float> pill (pillArea.getX(), pillArea.getCentreY() - 8.0f, 46.0f, 16.0f);
            juce::ColourGradient pillGrad (juce::Colour (0xffc7cdd4), pill.getX(), pill.getY(), juce::Colour (0xff9aa1ab), pill.getX(), pill.getBottom(), false);
            g.setGradientFill (pillGrad);
            g.fillRoundedRectangle (pill, 8.0f);

            inner.removeFromLeft (10.0f);

            auto* hub = SceneManagerHub::getInstance();
            bool killed = hub->globalKillSwitch.load();
            bool locked = hub->isLocked.load();

            auto ledArea = inner.removeFromLeft (34.0f);
            float ledW = 12.0f, ledH = clusterH - 14.0f;
            bool ledOn[2] = { killed, locked };
            juce::Colour ledOnColour[2] = { DarkFaderColours::fatherRed, juce::Colour (0xff4fbf6a) };

            for (int i = 0; i < 2; ++i)
            {
                juce::Rectangle<float> led (ledArea.getX() + i * (ledW + 5.0f), ledArea.getCentreY() - ledH * 0.5f, ledW, ledH);
                juce::Colour c = ledOn[i] ? ledOnColour[i] : juce::Colour (0xff3a3d43);
                if (ledOn[i])
                {
                    juce::DropShadow ledGlow (c.withAlpha (0.8f), 6, {0, 0});
                    juce::Path lp; lp.addRoundedRectangle (led, 2.5f);
                    ledGlow.drawForPath (g, lp);
                }
                g.setColour (c);
                g.fillRoundedRectangle (led, 2.5f);
            }

            inner.removeFromLeft (10.0f);

            int activeIdx = hub->activeScene.load();
            juce::String sceneLabel = juce::String (hub->getSceneNumber (activeIdx)).paddedLeft ('0', 2)
                                       + juce::String (" - ") + hub->getSceneName (activeIdx).toUpperCase();
            g.setFont (getDarkFaderFont (12.5f));
            g.setColour (DarkFaderColours::ink);
            g.drawText (sceneLabel, inner.toNearestInt(), juce::Justification::centredLeft, true);
        }

        drawFaderStrip (g);
    }

    // Version Number
    g.setFont (getDarkFaderFont (10.0f));
    g.setColour (DarkFaderColours::inkFaint);
    g.drawText("v" + juce::String (JucePlugin_VersionString), getLocalBounds().reduced(4), juce::Justification::bottomRight, false);
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
        int rColY = 118; // shifted down to clear the chest-panel status cluster
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
        
        childrenRowLabel.setBounds(startX - 78, 130 + sceneRowH + 12, 72, 24);

        int numRows = (activeScenes + 4) / maxCols;
        int gridBottom = 130 + (numRows - 1) * 150 + sceneRowH + 7 + 62;
        faderStripArea = juce::Rectangle<int> (24, gridBottom + 24, bounds.getWidth() - 96 - 24, faderStripHeight);
        
        for (int i = 0; i < activeScenes; ++i)
        {
            if (sceneButtons[i]->isVisible()) {
                int row = i / maxCols;
                int col = i % maxCols;
                
                int sceneX = startX + col * btnW + 2;
                int sceneY = 130 + row * 150; // shifted down to clear the chest-panel status cluster
                
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
        
        gainKnob.setBounds(bounds.getCentreX() - 60, 156, 120, 206);
        childNameLabel.setBounds(0, 368, bounds.getWidth(), 34);
    }
}

// One fader per Child, drawn with the same fader and lamp as the Child view.
// Children that aren't reporting audio (not loaded, or a spare slot) are
// drawn dimmed at their scene target instead.
void DarkFaderAudioProcessorEditor::drawFaderStrip (juce::Graphics& g)
{
    if (faderStripArea.isEmpty())
        return;

    auto* hub = SceneManagerHub::getInstance();
    auto strip = faderStripArea.toFloat();

    g.setColour (juce::Colour (0xff101113));
    g.fillRoundedRectangle (strip, 10.0f);
    g.setColour (DarkFaderColours::edgeFather);
    g.drawRoundedRectangle (strip, 10.0f, 1.0f);

    g.setFont (getDarkFaderFont (10.5f));
    g.setColour (DarkFaderColours::inkFaint);
    g.drawText ("LIVE LEVELS", strip.reduced (14.0f, 8.0f).removeFromTop (14.0f).toNearestInt(),
                juce::Justification::centredLeft, false);

    auto inner = strip.reduced (8.0f, 0.0f).withTrimmedTop (24.0f).withTrimmedBottom (8.0f);
    float colW = inner.getWidth() / (float) MAX_CHILDREN;
    bool killed = hub->globalKillSwitch.load();

    for (int s = 0; s < MAX_CHILDREN; ++s)
    {
        auto col = juce::Rectangle<float> (inner.getX() + s * colW, inner.getY(), colW, inner.getHeight());

        bool live = hub->isChildLive (s);
        juce::String name = hub->getChildName (s);
        bool spare = !live && name.isEmpty();

        float level = live ? hub->getChildLevel (s) : hub->getTargetGainForScene (s, hub->activeScene.load());
        if (level < 0.0f) level = 0.0f; // "leave it" with no live data - nothing to show
        if (killed && live) level = 1.0f;
        level = juce::jlimit (0.0f, 1.0f, level);
        float alpha = live ? 1.0f : 0.35f;

        auto lampArea = col.removeFromTop (18.0f);
        float lampW = juce::jmin (52.0f, colW - 10.0f);
        DarkFaderFader::drawLamp (g, lampArea.withSizeKeepingCentre (lampW, 16.0f), level, alpha, spare);

        auto badgeArea = col.removeFromBottom (18.0f);
        auto nameArea = col.removeFromBottom (26.0f);
        col.removeFromTop (6.0f);
        col.removeFromBottom (4.0f);

        DarkFaderFader::drawFader (g, col, level, alpha, juce::jmin (36.0f, colW * 0.7f));

        // Name (track name with any trailing " - n" dropped, since the badge shows the number)
        juce::String shownName = spare ? juce::String ("SPARE") : name;
        if (shownName.isEmpty()) shownName = "CHILD " + juce::String (s + 1);
        auto dash = shownName.lastIndexOf (" - ");
        if (dash > 0 && shownName.substring (dash + 3).trim().containsOnly ("0123456789"))
            shownName = shownName.substring (0, dash);
        g.setFont (getDarkFaderFont (10.5f));
        g.setColour (DarkFaderColours::fatherSilver.withMultipliedAlpha (alpha));
        g.drawFittedText (shownName.toUpperCase(), nameArea.reduced (2.0f, 0.0f).toNearestInt(), juce::Justification::centred, 2, 0.8f);

        // Bronze badge, matching the Child view's ID plate
        juce::String badge = "CHILD " + juce::String (s + 1);
        g.setFont (getDarkFaderFont (9.5f));
        float bw = juce::jmin (colW - 6.0f, (float) g.getCurrentFont().getStringWidth (badge) + 14.0f);
        juce::Rectangle<float> plate (col.getCentreX() - bw * 0.5f, badgeArea.getY() + 1.0f, bw, 16.0f);
        g.setColour (juce::Colour (0xff100d09).withMultipliedAlpha (alpha));
        g.fillRoundedRectangle (plate, 5.0f);
        g.setColour (DarkFaderColours::childBronze.withMultipliedAlpha (alpha));
        g.drawRoundedRectangle (plate, 5.0f, 1.0f);
        g.drawText (badge, plate.toNearestInt(), juce::Justification::centred, false);
    }
}
