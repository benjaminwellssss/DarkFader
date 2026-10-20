cmake_minimum_required(VERSION 3.20)
project(DarkFader VERSION 1.0.5)

# Target Apple Silicon and Intel processors for macOS Universal Binary
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for Mac" FORCE)
endif()

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    juce
    URL https://github.com/juce-framework/JUCE/archive/refs/tags/7.0.9.zip
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(DarkFader
    COMPANY_NAME "BeenWell"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE "Bwel"
    PLUGIN_CODE "Dfad"
    FORMATS VST3 AU Standalone
)

juce_generate_juce_header(DarkFader)

target_sources(DarkFader PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SceneManagerHub.cpp
)

target_compile_definitions(DarkFader PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

juce_add_binary_data(DarkFader_BinaryData
    HEADER_NAME BinaryData.h
    NAMESPACE BinaryData
    SOURCES
        Assets/knob.png
        Assets/DarkFader.svg
        Assets/Montserrat-Bold.ttf
)

target_link_libraries(DarkFader PRIVATE
    juce::juce_audio_utils
    DarkFader_BinaryData
    juce::juce_audio_processors
    juce::juce_gui_extra
)

# Demo rig: one Father + eight Children in a single app, for testing the UI
# without Logic. Off by default and never part of the installer.
#   cmake -S . -B build -DDARKFADER_DEMO_RIG=ON && cmake --build build --target DarkFaderDemoRig
option(DARKFADER_DEMO_RIG "Build the DarkFader demo rig app" OFF)
if(DARKFADER_DEMO_RIG)
    juce_add_gui_app(DarkFaderDemoRig PRODUCT_NAME "DarkFader Demo Rig" COMPANY_NAME "BeenWell")
    juce_generate_juce_header(DarkFaderDemoRig)
    target_sources(DarkFaderDemoRig PRIVATE
        Tools/DemoRig/Main.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/SceneManagerHub.cpp
    )
    target_compile_definitions(DarkFaderDemoRig PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JucePlugin_Name="DarkFader"
        JucePlugin_VersionString="${PROJECT_VERSION}"
        JucePlugin_IsSynth=0
        JucePlugin_IsMidiEffect=0
        JucePlugin_WantsMidiInput=0
        JucePlugin_ProducesMidiOutput=0
    )
    target_link_libraries(DarkFaderDemoRig PRIVATE
        DarkFader_BinaryData
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_extra
    )

    # Regression test for jump-free fades: cmake --build build --target DarkFaderRampTest && build/DarkFaderRampTest_artefacts/Release/DarkFaderRampTest
    juce_add_console_app(DarkFaderRampTest PRODUCT_NAME "DarkFaderRampTest")
    juce_generate_juce_header(DarkFaderRampTest)
    target_sources(DarkFaderRampTest PRIVATE Tools/RampTest/Main.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/SceneManagerHub.cpp)
    target_compile_definitions(DarkFaderRampTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="DarkFader" JucePlugin_VersionString="${PROJECT_VERSION}"
        JucePlugin_IsSynth=0 JucePlugin_IsMidiEffect=0 JucePlugin_WantsMidiInput=0 JucePlugin_ProducesMidiOutput=0)
    target_link_libraries(DarkFaderRampTest PRIVATE DarkFader_BinaryData juce::juce_audio_utils juce::juce_audio_processors juce::juce_gui_extra)

    # Renders plugin windows to PNG for the training guide: build/DarkFaderSnapshots_artefacts/Release/DarkFaderSnapshots <outDir> [preset]
    juce_add_console_app(DarkFaderSnapshots PRODUCT_NAME "DarkFaderSnapshots")
    juce_generate_juce_header(DarkFaderSnapshots)
    target_sources(DarkFaderSnapshots PRIVATE Tools/Snapshots/Main.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/SceneManagerHub.cpp)
    target_compile_definitions(DarkFaderSnapshots PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="DarkFader" JucePlugin_VersionString="${PROJECT_VERSION}"
        JucePlugin_IsSynth=0 JucePlugin_IsMidiEffect=0 JucePlugin_WantsMidiInput=0 JucePlugin_ProducesMidiOutput=0)
    target_link_libraries(DarkFaderSnapshots PRIVATE DarkFader_BinaryData juce::juce_audio_utils juce::juce_audio_processors juce::juce_gui_extra)
endif()
