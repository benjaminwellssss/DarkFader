#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <array>
#include <unordered_map>

// Max Number of scenes
constexpr int MAX_SCENES = 32;
// Max number of children we can manage (fixed size for lock-free array)
constexpr int MAX_CHILDREN = 10;

struct ChildInfo {
    std::atomic<bool> isActive{false};
    std::atomic<float> gainValues[MAX_SCENES];
    juce::Colour color{juce::Colours::white};
    juce::String name;

    ChildInfo() {
        for (int i = 0; i < MAX_SCENES; ++i)
            gainValues[i].store(0.0f);
    }
};

class SceneManagerHub
{
public:
    juce_DeclareSingleton(SceneManagerHub, false)

    SceneManagerHub();
    ~SceneManagerHub();

    // Global kill switch checked by all DSP threads
    std::atomic<bool> globalKillSwitch { false };

    // Currently active scene index
    std::atomic<int> activeScene { 0 };

    // Dynamic scene management
    std::atomic<int> numActiveScenes { 5 };
    std::atomic<bool> isLocked { true };
    std::atomic<float> globalFadeTime { 1.5f };

    // Update gain value for a child in a specific scene (called from UI)
    float getTargetGain(int childId) const;
    float getTargetGainForScene(int childId, int sceneIndex) const;

    // Update gain value for a child in a specific scene (called from UI)
    void setTargetGain(int childId, int sceneIndex, float newGain);

    // Get color and name for UI
    juce::Colour getChildColor(int childId) const;
    juce::String getChildName(int childId) const;


    // Scene Customization
    juce::String getSceneName(int index) const;
    void setSceneName(int index, const juce::String& newName);
    int getSceneNumber(int index) const;
    void setSceneNumber(int index, int newNumber);

    juce::Colour getSceneColor(int index) const;
    void setSceneColor(int index, juce::Colour newColor);

    void resetScene(int index);

    void moveScene(int fromIndex, int toIndex);

private:
    std::array<ChildInfo, MAX_CHILDREN> children;
    juce::String sceneNames[MAX_SCENES];
    int sceneNumbers[MAX_SCENES];
    juce::Colour sceneColors[MAX_SCENES];
    mutable juce::CriticalSection uiLock;   // Used for strings/customization
};
