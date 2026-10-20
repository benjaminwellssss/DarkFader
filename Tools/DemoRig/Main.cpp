// DarkFader Demo Rig
// Runs one Father and eight Children in a single process (the way Logic hosts
// them), feeds the Children a quiet test tone in real time, and opens the
// Father plus two Child windows. Lets you watch the Father's live fader strip
// without opening Logic. Not part of the shipped plugin.
//
// Usage: DarkFaderDemoRig.app [path/to/preset.puppet]
//        (defaults to ~/Desktop/sunday.puppet if it exists)

#include <JuceHeader.h>
#include "../../Source/PluginProcessor.h"
#include "../../Source/SceneManagerHub.h"

namespace
{
    const char* busNames[] = { "SINGERS - 1", "BAND - 2", "DRUMS - 3", "E GUITARS - 4",
                               "PASTORS - 5", "MAC - 6", "CROWD - 7", "ALL FX - 8" };
    constexpr int numChildren = 8;
    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 512;
}

// Calls every Child's processBlock at real-time pace, like a host's audio thread.
class FakeAudioThread : public juce::Thread
{
public:
    explicit FakeAudioThread (juce::OwnedArray<DarkFaderAudioProcessor>& p) : juce::Thread ("DemoAudio"), procs (p) {}

    void run() override
    {
        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;
        double phase = 0.0;
        const double blockMs = 1000.0 * blockSize / sampleRate;
        double next = juce::Time::getMillisecondCounterHiRes();

        while (! threadShouldExit())
        {
            for (auto* p : procs)
            {
                for (int i = 0; i < blockSize; ++i)
                {
                    float v = 0.05f * (float) std::sin (phase + i * juce::MathConstants<double>::twoPi * 220.0 / sampleRate);
                    buffer.setSample (0, i, v);
                    buffer.setSample (1, i, v);
                }
                p->processBlock (buffer, midi);
            }
            phase += blockSize * juce::MathConstants<double>::twoPi * 220.0 / sampleRate;

            next += blockMs;
            auto wait = next - juce::Time::getMillisecondCounterHiRes();
            if (wait > 0) juce::Thread::sleep ((int) wait);
        }
    }

private:
    juce::OwnedArray<DarkFaderAudioProcessor>& procs;
};

class PluginWindow : public juce::DocumentWindow
{
public:
    PluginWindow (const juce::String& title, juce::AudioProcessorEditor* editor, int x, int y)
        : juce::DocumentWindow (title, juce::Colours::black, juce::DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (editor, true);
        setTopLeftPosition (x, y);
        setVisible (true);
    }
    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
};

// Small control panel: auto-steps through the service so the fades can be watched.
class DemoControls : public juce::Component, private juce::Timer
{
public:
    DemoControls()
    {
        autoButton.setClickingTogglesState (true);
        autoButton.onClick = [this] {
            if (autoButton.getToggleState()) { advance(); startTimer (5000); }
            else stopTimer();
        };
        nextButton.onClick = [this] { advance(); };
        addAndMakeVisible (autoButton);
        addAndMakeVisible (nextButton);
        info.setText ("Feeding a test tone through 8 busses.", juce::dontSendNotification);
        addAndMakeVisible (info);
        setSize (360, 110);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced (12);
        info.setBounds (b.removeFromTop (28));
        b.removeFromTop (8);
        autoButton.setBounds (b.removeFromLeft (b.getWidth() / 2 - 4));
        b.removeFromLeft (8);
        nextButton.setBounds (b);
    }

private:
    void timerCallback() override { advance(); }
    void advance()
    {
        auto* hub = SceneManagerHub::getInstance();
        // Step through the service scenes only (skip ALL ON / ALL OFF test scenes at the end).
        int serviceScenes = juce::jmin (8, hub->numActiveScenes.load());
        int next = (hub->activeScene.load() + 1) % juce::jmax (1, serviceScenes);
        hub->activeScene.store (next);
        info.setText ("Now: " + hub->getSceneName (next), juce::dontSendNotification);
    }

    juce::TextButton autoButton { "Auto-play service" }, nextButton { "Next scene" };
    juce::Label info;
};

class DemoRigApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "DarkFader Demo Rig"; }
    const juce::String getApplicationVersion() override { return "1.0"; }

    void initialise (const juce::String& commandLine) override
    {
        // Father
        father = std::make_unique<DarkFaderAudioProcessor>();
        father->isFather = true;

        juce::File preset (commandLine.unquoted().trim());
        if (! preset.existsAsFile())
            preset = juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile ("sunday.puppet");
        if (preset.existsAsFile())
        {
            juce::MemoryBlock mb;
            preset.loadFileAsData (mb);
            father->setStateInformation (mb.getData(), (int) mb.getSize());
        }

        // Children, one per bus, plus one left unassigned
        for (int k = 0; k < numChildren; ++k)
        {
            auto* c = children.add (new DarkFaderAudioProcessor());
            c->isFather = false;
            c->isChildLocked = true;
            c->childId = k;
            c->customName = busNames[k];
            SceneManagerHub::getInstance()->setChildName (k, busNames[k]);
        }
        for (auto* p : children)
        {
            p->setRateAndBufferSizeDetails (sampleRate, blockSize);
            p->prepareToPlay (sampleRate, blockSize);
        }

        unassigned = std::make_unique<DarkFaderAudioProcessor>();
        unassigned->isFather = false;
        unassigned->customName = "NEW BUS";

        audio = std::make_unique<FakeAudioThread> (children);
        audio->startThread (juce::Thread::Priority::high);

        fatherWindow   = std::make_unique<PluginWindow> ("Father (Logic: Bus 256)", father->createEditor(), 40, 60);
        childWindow    = std::make_unique<PluginWindow> ("Child: SINGERS - 1", children[0]->createEditor(), 40, 820);
        unassignedWin  = std::make_unique<PluginWindow> ("Child: unassigned", unassigned->createEditor(), 420, 820);

        controlsWindow = std::make_unique<juce::DocumentWindow> ("Demo controls", juce::Colours::darkgrey, juce::DocumentWindow::closeButton);
        controlsWindow->setUsingNativeTitleBar (true);
        controlsWindow->setContentOwned (new DemoControls(), true);
        controlsWindow->setTopLeftPosition (800, 820);
        controlsWindow->setVisible (true);
    }

    void shutdown() override
    {
        if (audio) audio->stopThread (2000);
        controlsWindow = nullptr; unassignedWin = nullptr; childWindow = nullptr; fatherWindow = nullptr;
        unassigned = nullptr; children.clear(); father = nullptr;
        SceneManagerHub::deleteInstance();
    }

private:
    std::unique_ptr<DarkFaderAudioProcessor> father, unassigned;
    juce::OwnedArray<DarkFaderAudioProcessor> children;
    std::unique_ptr<FakeAudioThread> audio;
    std::unique_ptr<PluginWindow> fatherWindow, childWindow, unassignedWin;
    std::unique_ptr<juce::DocumentWindow> controlsWindow;
};

START_JUCE_APPLICATION (DemoRigApp)
