#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "SceneManagerHub.h"

class DarkFaderAudioProcessor  : public juce::AudioProcessor
{
public:
    DarkFaderAudioProcessor();
    ~DarkFaderAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    void updateTrackProperties(const TrackProperties& properties) override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- DarkFader Specifics ---

    // Toggle for Father/Child (true = Father, false = Child). Atomic since
    // it's written from UI clicks and read every audio block.
    std::atomic<bool> isFather { true };
    std::atomic<bool> isChildLocked { false };

    // Our assigned slot from the SceneManagerHub, or -1 if this Child
    // instance hasn't been assigned one yet (shows "UNASSIGNED" in the UI
    // and passes audio through untouched at unity gain until it is).
    std::atomic<int> childId { -1 };


    // Linear gain ramp that always starts from wherever the level is right
    // now. (juce::SmoothedValue::reset() snaps to the old target when the ramp
    // length changes, which made busses jump if a scene changed mid-fade.)
    struct GainRamp
    {
        float current = 1.0f, target = 1.0f, step = 0.0f;
        int stepsLeft = 0;

        void snap (float v) noexcept { current = target = v; step = 0.0f; stepsLeft = 0; }

        void rampTo (float newTarget, int numSteps) noexcept
        {
            if (newTarget == target)
                return; // already there or already heading there
            target = newTarget;
            if (numSteps <= 1) { snap (newTarget); return; }
            stepsLeft = numSteps;
            step = (target - current) / (float) numSteps;
        }

        float next() noexcept
        {
            if (stepsLeft > 0)
            {
                current += step;
                if (--stepsLeft == 0)
                    current = target;
            }
            return current;
        }
    };

    GainRamp gainRamp;

    // Level for the UI, written once per audio block.
    std::atomic<float> currentGainForUi { 1.0f };
    float getCurrentGain() const noexcept { return currentGainForUi.load (std::memory_order_relaxed); }

    // Set by the Child view while the user drags its fader, so the audio
    // follows the hand quickly instead of using the slow scene fade time.
    std::atomic<juce::uint32> lastManualEditMs { 0 };
    
    // Exposed for the editor to read/write temporarily before passing to the Hub
    // (In a full plugin we would use APVTS, but we keep it simple here)
    juce::String customName = "CHILD FADER";
    juce::Colour customColor = juce::Colours::cyan;
    
    // For child view local knob
    float currentLocalGain = 1.0f;
    int lastActiveScene = -1;


private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkFaderAudioProcessor)
};
