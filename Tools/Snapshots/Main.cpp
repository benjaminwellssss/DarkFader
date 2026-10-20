// Renders DarkFader windows to PNG without a screen (for the training guide).
// Usage: DarkFaderSnapshots <outDir> [preset.puppet]
#include <JuceHeader.h>
#include "../../Source/PluginProcessor.h"
// Test tool only: reach the editor's timer callback so the UI state updates without an event loop.
#define private public
#include "../../Source/PluginEditor.h"
#undef private
#include "../../Source/SceneManagerHub.h"

static const char* busNames[] = { "SINGERS - 1", "BAND - 2", "DRUMS - 3", "E GUITARS - 4", "PASTORS - 5", "MAC - 6", "CROWD - 7", "ALL FX - 8" };

static void save (juce::Component& c, const juce::File& f)
{
    // Hide JUCE's start-up splash logo (it fades away in the real plugin after a few seconds).
    for (auto* child : c.getChildren())
        if (dynamic_cast<juce::JUCESplashScreen*> (child) != nullptr)
            child->setVisible (false);
    auto img = c.createComponentSnapshot (c.getLocalBounds(), true, 2.0f);
    f.deleteFile();
    juce::FileOutputStream out (f);
    juce::PNGImageFormat().writeImageToStream (img, out);
    std::printf ("wrote %s (%dx%d)\n", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    std::setvbuf (stdout, nullptr, _IONBF, 0);
    juce::File outDir (argc > 1 ? argv[1] : "/tmp");
    outDir.createDirectory();
    juce::File preset (argc > 2 ? argv[2] : juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile ("sunday.puppet").getFullPathName());

    std::printf ("init ok\n");
    DarkFaderAudioProcessor father; father.isFather = true;
    std::printf ("father ok\n");
    if (preset.existsAsFile()) { juce::MemoryBlock mb; preset.loadFileAsData (mb); father.setStateInformation (mb.getData(), (int) mb.getSize()); }
    auto* hub = SceneManagerHub::getInstance();
    hub->globalFadeTime = 2.5f; // the live template uses 2.5 s

    juce::OwnedArray<DarkFaderAudioProcessor> kids;
    const double sr = 48000.0; const int bs = 512;
    for (int k = 0; k < 8; ++k)
    {
        auto* c = kids.add (new DarkFaderAudioProcessor());
        c->isFather = false; c->isChildLocked = true; c->childId = k; c->customName = busNames[k];
        hub->setChildName (k, busNames[k]);
        c->setRateAndBufferSizeDetails (sr, bs); c->prepareToPlay (sr, bs);
    }
    juce::AudioBuffer<float> buf (2, bs); juce::MidiBuffer midi;
    auto run = [&] (double seconds) {
        int n = (int) (seconds * sr / bs);
        for (int i = 0; i < n; ++i) for (auto* c : kids) { buf.clear(); c->processBlock (buf, midi); }
    };

    std::printf ("kids ok\n");
    std::unique_ptr<juce::AudioProcessorEditor> fe (father.createEditor());
    std::printf ("editor ok\n");
    auto tickF = [&] { static_cast<DarkFaderAudioProcessorEditor*> (fe.get())->timerCallback(); };
    auto shot = [&] (const char* name) {
        tickF();
        save (*fe, outDir.getChildFile (name));
    };

    hub->activeScene = 0; run (0.1);  shot ("df-father-preservice.png");
    hub->activeScene = 1; run (1.0);  shot ("df-father-midfade.png");   // ~40% into the WORSHIP fade
    run (2.0);                         shot ("df-father-worship.png");
    hub->globalKillSwitch = true; run (0.05); shot ("df-father-kill.png");
    hub->globalKillSwitch = false; run (3.0);

    // Child views: assigned at a partial level, and unassigned
    hub->setTargetGain (0, 1, 0.6f); run (3.0);
    std::unique_ptr<juce::AudioProcessorEditor> ce (kids[0]->createEditor());
    auto tick = [&] { static_cast<DarkFaderAudioProcessorEditor*> (ce.get())->timerCallback(); };
    tick();
    save (*ce, outDir.getChildFile ("df-child-60.png"));
    hub->setTargetGain (0, 1, 1.0f); run (3.0);
    tick();
    save (*ce, outDir.getChildFile ("df-child-on.png"));

    DarkFaderAudioProcessor lone; lone.isFather = false; lone.customName = "NEW BUS";
    std::unique_ptr<juce::AudioProcessorEditor> ue (lone.createEditor());
    static_cast<DarkFaderAudioProcessorEditor*> (ue.get())->timerCallback();
    save (*ue, outDir.getChildFile ("df-child-unassigned.png"));

    ce = nullptr; ue = nullptr; fe = nullptr;
    SceneManagerHub::deleteInstance();
    return 0;
}
