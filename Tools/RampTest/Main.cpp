// Checks that a Child never jumps: switches scenes mid-fade and simulates a
// fader drag, and fails if the level changes faster than a fade allows.
#include <JuceHeader.h>
#include "../../Source/PluginProcessor.h"
#include "../../Source/SceneManagerHub.h"

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    auto* hub = SceneManagerHub::getInstance();
    hub->numActiveScenes = 3;
    hub->globalFadeTime = 2.5f;
    hub->setTargetGain (0, 0, 1.0f);   // scene 0: on
    hub->setTargetGain (0, 1, 0.0f);   // scene 1: off
    hub->setTargetGain (0, 2, 1.0f);   // scene 2: on
    hub->activeScene = 0;

    DarkFaderAudioProcessor child;
    child.isFather = false; child.childId = 0;
    const double sr = 48000.0; const int bs = 512;
    child.setRateAndBufferSizeDetails (sr, bs); child.prepareToPlay (sr, bs);

    juce::AudioBuffer<float> buf (2, bs); juce::MidiBuffer midi;
    float last = -1.0f, maxJump = 0.0f; int blocks = 0;
    auto run = [&] (int n) {
        for (int i = 0; i < n; ++i) {
            buf.clear(); for (int s = 0; s < bs; ++s) { buf.setSample (0, s, 1.0f); buf.setSample (1, s, 1.0f); }
            child.processBlock (buf, midi);
            float lvl = child.getCurrentGain();
            if (last >= 0.0f) maxJump = std::max (maxJump, std::abs (lvl - last));
            last = lvl; ++blocks;
        }
    };

    run (5);                                   // settle at ON (snap on first block is expected)
    maxJump = 0.0f;
    hub->activeScene = 1; run (60);            // ~0.64 s into a 2.5 s fade down
    float mid = last;
    hub->activeScene = 2; run (300);           // change scene MID-FADE back up
    const float allowedSceneStep = (float) (bs / (2.5 * sr)) * 1.01f; // one block of a full-range 2.5 s fade
    std::printf ("scene test: level mid-fade %.3f, biggest per-block change %.4f (allowed %.4f)\n", mid, maxJump, allowedSceneStep);
    bool ok = maxJump <= allowedSceneStep && mid > 0.5f && mid < 0.95f;

    // Fader drag: target changes every block by small amounts -> must follow quickly, no jumps beyond the drag step
    maxJump = 0.0f;
    for (int i = 0; i < 40; ++i) {
        child.lastManualEditMs = juce::Time::getMillisecondCounter();
        hub->setTargetGain (0, 2, 1.0f - 0.01f * (float) i);
        run (1);
    }
    run (10);
    std::printf ("drag test: ended at %.3f (target 0.610), biggest per-block change %.4f\n", last, maxJump);
    ok = ok && std::abs (last - 0.61f) < 0.01f && maxJump < 0.02f;

    std::printf (ok ? "PASS\n" : "FAIL\n");
    SceneManagerHub::deleteInstance();
    return ok ? 0 : 1;
}
