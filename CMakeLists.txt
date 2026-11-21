cmake_minimum_required(VERSION 3.22)

project(IcoKickOne VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(IcoKickOne
    COMPANY_NAME "Independent Audio Tools"
    BUNDLE_ID "audio.independent.icokickone"
    PLUGIN_MANUFACTURER_CODE InAu
    PLUGIN_CODE IcK1
    PRODUCT_NAME "IcoKickOne"
    DESCRIPTION "Original procedural techno kick instrument"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    FORMATS VST3
)

juce_generate_juce_header(IcoKickOne)

target_sources(IcoKickOne
    PRIVATE
        include/IcoKickOne/KickDsp.h
        src/dsp/KickDsp.cpp
        src/plugin/PluginProcessor.cpp
        src/plugin/PluginProcessor.h
        src/ui/PluginEditor.h
        src/ui/PluginEditor.cpp
)

target_include_directories(IcoKickOne PRIVATE include src/plugin src/ui)
target_compile_features(IcoKickOne PRIVATE cxx_std_17)

target_compile_definitions(IcoKickOne
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(IcoKickOne
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
