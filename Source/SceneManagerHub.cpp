#include "SceneManagerHub.h"

juce_ImplementSingleton(SceneManagerHub)

SceneManagerHub::SceneManagerHub()
{
    for (int i = 0; i < MAX_SCENES; ++i)
    {
        sceneNames[i] = "SCENE " + juce::String(i + 1);
        sceneNumbers[i] = i + 1;
        sceneColors[i] = juce::Colour::fromHSV(juce::jlimit(0.0f, 1.0f, (float)i / 8.0f), 0.7f, 0.9f, 1.0f);
    }
}

SceneManagerHub::~SceneManagerHub()
{
}


float SceneManagerHub::getTargetGain(int childId) const
{
    if (childId >= 0 && childId < MAX_CHILDREN)
    {
        int currentScene = activeScene.load();
        return children[childId].gainValues[currentScene].load();
    }
    return 1.0f; // Default if not found
}

float SceneManagerHub::getTargetGainForScene(int childId, int sceneIndex) const
{
    if (childId >= 0 && childId < MAX_CHILDREN && sceneIndex >= 0 && sceneIndex < MAX_SCENES)
    {
        return children[childId].gainValues[sceneIndex].load();
    }
    return 0.0f;
}

void SceneManagerHub::setTargetGain(int childId, int sceneIndex, float newGain)
{
    if (childId >= 0 && childId < MAX_CHILDREN && sceneIndex >= 0 && sceneIndex < MAX_SCENES)
    {
        children[childId].gainValues[sceneIndex].store(newGain);
    }
}

juce::Colour SceneManagerHub::getChildColor(int childId) const
{
    if (childId >= 0 && childId < MAX_CHILDREN)
        return children[childId].color;
    return juce::Colours::white;
}

juce::String SceneManagerHub::getChildName(int childId) const
{
    if (childId >= 0 && childId < MAX_CHILDREN)
        return children[childId].name;
    return "Unknown";
}


juce::String SceneManagerHub::getSceneName(int index) const
{
    if (index >= 0 && index < MAX_SCENES)
    {
        juce::ScopedLock sl(uiLock);
        return sceneNames[index];
    }
    return {};
}

void SceneManagerHub::setSceneName(int index, const juce::String& newName)
{
    if (index >= 0 && index < MAX_SCENES)
    {
        juce::ScopedLock sl(uiLock);
        sceneNames[index] = newName;
    }
}

int SceneManagerHub::getSceneNumber(int index) const
{
    if (index >= 0 && index < MAX_SCENES)
    {
        juce::ScopedLock sl(uiLock);
        return sceneNumbers[index];
    }
    return index + 1;
}

void SceneManagerHub::setSceneNumber(int index, int newNumber)
{
    if (index >= 0 && index < MAX_SCENES)
    {
        juce::ScopedLock sl(uiLock);
        sceneNumbers[index] = newNumber;
    }
}


juce::Colour SceneManagerHub::getSceneColor(int index) const
{
    if (index >= 0 && index < MAX_SCENES)
    {
        juce::ScopedLock sl(uiLock);
        return sceneColors[index];
    }
    return juce::Colours::white;
}

void SceneManagerHub::setSceneColor(int index, juce::Colour newColor)
{
    if (index >= 0 && index < MAX_SCENES)
    {
        const juce::ScopedLock sl(uiLock);
        sceneColors[index] = newColor;
    }
}

void SceneManagerHub::resetScene(int index)
{
    if (index >= 0 && index < MAX_SCENES)
    {
        juce::ScopedLock sl(uiLock);
        sceneNames[index] = "SCENE " + juce::String(index + 1);
        sceneNumbers[index] = index + 1;
        sceneColors[index] = juce::Colour::fromHSV(juce::jlimit(0.0f, 1.0f, (float)index / 8.0f), 0.7f, 0.9f, 1.0f);
        
        for (int s = 0; s < MAX_CHILDREN; ++s) {
            children[s].gainValues[index].store(0.0f);
        }
    }
}

void SceneManagerHub::moveScene(int fromIndex, int toIndex)
{
    if (fromIndex == toIndex || fromIndex < 0 || fromIndex >= MAX_SCENES || toIndex < 0 || toIndex >= MAX_SCENES)
        return;

    const juce::ScopedLock sl(uiLock);

    // Save moving data
    juce::String movedName = sceneNames[fromIndex];
    int movedNumber = sceneNumbers[fromIndex];
    juce::Colour movedColor = sceneColors[fromIndex];

    float movedGains[MAX_CHILDREN];
    for (int s = 0; s < MAX_CHILDREN; ++s) {
        movedGains[s] = children[s].gainValues[fromIndex].load();
    }

    // Shift data
    if (fromIndex < toIndex) {
        // Shift left
        for (int i = fromIndex; i < toIndex; ++i) {
            sceneNames[i] = sceneNames[i + 1];
            sceneNumbers[i] = sceneNumbers[i + 1];
            sceneColors[i] = sceneColors[i + 1];
            for (int s = 0; s < MAX_CHILDREN; ++s) {
                children[s].gainValues[i].store(children[s].gainValues[i + 1].load());
            }
        }
    } else {
        // Shift right
        for (int i = fromIndex; i > toIndex; --i) {
            sceneNames[i] = sceneNames[i - 1];
            sceneNumbers[i] = sceneNumbers[i - 1];
            sceneColors[i] = sceneColors[i - 1];
            for (int s = 0; s < MAX_CHILDREN; ++s) {
                children[s].gainValues[i].store(children[s].gainValues[i - 1].load());
            }
        }
    }

    // Insert moving data
    sceneNames[toIndex] = movedName;
    sceneNumbers[toIndex] = movedNumber;
    sceneColors[toIndex] = movedColor;
    for (int s = 0; s < MAX_CHILDREN; ++s) {
        children[s].gainValues[toIndex].store(movedGains[s]);
    }

    // Update activeScene if it was affected
    int currentActive = activeScene.load();
    if (currentActive == fromIndex) {
        activeScene.store(toIndex);
    } else if (fromIndex < toIndex && currentActive > fromIndex && currentActive <= toIndex) {
        activeScene.store(currentActive - 1);
    } else if (fromIndex > toIndex && currentActive >= toIndex && currentActive < fromIndex) {
        activeScene.store(currentActive + 1);
    }
}
