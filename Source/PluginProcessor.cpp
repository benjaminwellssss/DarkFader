#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <limits>

DarkFaderAudioProcessor::DarkFaderAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // Randomize initial color for variation
    juce::Random r;
    customColor = juce::Colour(r.nextInt(255), r.nextInt(255), r.nextInt(255)).withAlpha(1.0f);
    
    // Assign a default name
    customName = "Fader " + juce::String(r.nextInt(100));
}

DarkFaderAudioProcessor::~DarkFaderAudioProcessor()
{
}

const juce::String DarkFaderAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DarkFaderAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DarkFaderAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DarkFaderAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double DarkFaderAudioProcessor::getTailLengthSeconds() const
{
    return std::numeric_limits<double>::infinity();
}

int DarkFaderAudioProcessor::getNumPrograms()
{
    return 1;
}

int DarkFaderAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DarkFaderAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String DarkFaderAudioProcessor::getProgramName (int index)
{
    return {};
}

void DarkFaderAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void DarkFaderAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // 20ms smoothing time to prevent clicks on scene changes
    gainMultiplier.reset (sampleRate, 0.02);
    gainMultiplier.setCurrentAndTargetValue (1.0f);
}

void DarkFaderAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DarkFaderAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif


void DarkFaderAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto* hub = SceneManagerHub::getInstance();

    // Check global kill switch first
    if (hub->globalKillSwitch.load())
    {
        // Instantly snap to 1.0 (bypass)
        gainMultiplier.setCurrentAndTargetValue(1.0f);
        // Pass audio through untouched
        return;
    }

    if (isFather)
    {
        // Father does not process gain staging, it just passes audio through.
        return;
    }
    else
    {
        // Child processing
        if (childId != -1)
        {
            int currentScene = hub->activeScene.load();
            
            if (lastActiveScene == -1) {
                // Instantly snap to the target gain on the very first audio block (DAW load)
                float initialGain = hub->getTargetGain(childId);
                if (initialGain == -1.0f) initialGain = 1.0f;
                gainMultiplier.setCurrentAndTargetValue(initialGain);
                lastActiveScene = currentScene;
            }
            else if (currentScene != lastActiveScene) {
                float fadeTime = hub->globalFadeTime.load();
                // Apply the user's custom fade time ONLY when the scene actually changes
                gainMultiplier.reset(getSampleRate(), juce::jmax(0.001f, fadeTime));
                lastActiveScene = currentScene;
            }
            
            // Poll the hub for the target gain
            float targetGain = hub->getTargetGain(childId);
            if (targetGain != -1.0f) {
                gainMultiplier.setTargetValue(targetGain);
            }
        }
    }

    // Apply smoothed gain
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        
        // Use a copy of the smoother so channels stay perfectly in sync
        auto smootherCopy = gainMultiplier; 
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelData[sample] *= smootherCopy.getNextValue();
        }
    }
    
    // Advance the original smoother
    gainMultiplier.skip (buffer.getNumSamples());
}

bool DarkFaderAudioProcessor::hasEditor() const
{
    return true;
}

void DarkFaderAudioProcessor::updateTrackProperties(const TrackProperties& properties)
{
    juce::String newName = properties.name;
    if (newName.isNotEmpty() && newName != customName) {
        customName = newName;
        // The UI's label is updated every frame in PluginEditor's timerCallback, or we can just let it be drawn.
    }
}

juce::AudioProcessorEditor* DarkFaderAudioProcessor::createEditor()
{
    return new DarkFaderAudioProcessorEditor (*this);
}

void DarkFaderAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::XmlElement xml ("DARKFADER_STATE");
    xml.setAttribute("isFather", isFather);
    xml.setAttribute("isChildLocked", isChildLocked);
    xml.setAttribute("childId", childId);
    xml.setAttribute("customName", customName);
    xml.setAttribute("customColor", customColor.toString());

    if (isFather) {
        auto* hub = SceneManagerHub::getInstance();
        xml.setAttribute("numActiveScenes", hub->numActiveScenes.load());
        xml.setAttribute("isLocked", hub->isLocked.load());
        xml.setAttribute("globalFadeTime", hub->globalFadeTime.load());
        
        for (int i = 0; i < MAX_SCENES; ++i) {
            auto* sceneXml = new juce::XmlElement("SCENE");
            sceneXml->setAttribute("index", i);
            sceneXml->setAttribute("name", hub->getSceneName(i));
            sceneXml->setAttribute("number", hub->getSceneNumber(i));
            
            juce::String gainStr;
            for (int s = 0; s < MAX_CHILDREN; ++s) {
                gainStr << hub->getTargetGainForScene(s, i) << (s < MAX_CHILDREN - 1 ? "," : "");
            }
            sceneXml->setAttribute("grid", gainStr);
            
            xml.addChildElement(sceneXml);
        }
    }
    copyXmlToBinary(xml, destData);
}

void DarkFaderAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName("DARKFADER_STATE"))
        {
            isFather = xmlState->getBoolAttribute("isFather", isFather);
            isChildLocked = xmlState->getBoolAttribute("isChildLocked", isChildLocked);
            childId = xmlState->getIntAttribute("childId", childId);
            customName = xmlState->getStringAttribute("customName", customName);
            customColor = juce::Colour::fromString(xmlState->getStringAttribute("customColor", customColor.toString()));
            
            if (isFather) {
                auto* hub = SceneManagerHub::getInstance();
                hub->numActiveScenes.store(xmlState->getIntAttribute("numActiveScenes", 5));
                hub->isLocked.store(xmlState->getBoolAttribute("isLocked", false));
                hub->globalFadeTime.store(xmlState->getDoubleAttribute("globalFadeTime", 1.5));
                
                for (auto* child : xmlState->getChildIterator()) {
                    if (child->hasTagName("SCENE")) {
                        int idx = child->getIntAttribute("index");
                        hub->setSceneName(idx, child->getStringAttribute("name"));
                        hub->setSceneNumber(idx, child->getIntAttribute("number"));
                        
                        if (child->hasAttribute("grid")) {
                            juce::StringArray gains = juce::StringArray::fromTokens(child->getStringAttribute("grid"), ",", "");
                            for (int s = 0; s < MAX_CHILDREN && s < gains.size(); ++s) {
                                hub->setTargetGain(s, idx, gains[s].getFloatValue());
                            }
                        }
                    }
                }
            }
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DarkFaderAudioProcessor();
}
