cmake_minimum_required(VERSION 3.20)
project(DarkFader VERSION 1.0.3)

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
        Assets/child_bg.svg
        Assets/father_bg.svg
)

target_link_libraries(DarkFader PRIVATE
    juce::juce_audio_utils
    DarkFader_BinaryData
    juce::juce_audio_processors
    juce::juce_gui_extra
)
