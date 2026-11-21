cmake_minimum_required(VERSION 3.22)
project(Phraser VERSION 1.0.0 LANGUAGES CXX)

if (NOT WIN32)
    message(FATAL_ERROR "Phraser currently targets Windows only")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Phraser
    COMPANY_NAME "Phraser"
    BUNDLE_ID "com.picopicodragon.phraser"
    PLUGIN_MANUFACTURER_CODE PcDr
    PLUGIN_CODE Phrs
    FORMATS VST3
    PRODUCT_NAME "Phraser"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Instrument" "Sampler"
)

juce_generate_juce_header(Phraser)

target_sources(Phraser PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/TransientDetection.h
    Source/TempoDetection.h
    Source/GrainAlignment.h
    Source/PitchClassDetection.h
)

# JUCE/HarfBuzz source files contain UTF-8 characters that MSVC otherwise
# attempts to decode using the Japanese Windows default code page (CP932).
# /utf-8 enables both UTF-8 source and execution character sets.
if (MSVC)
    target_compile_options(Phraser PRIVATE /utf-8)
endif()

target_compile_definitions(Phraser PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Phraser PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
