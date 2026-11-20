cmake_minimum_required(VERSION 3.22)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(LunarEQ VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(LunarEQ
    COMPANY_NAME "AETHEREON AUDIO"
    BUNDLE_ID com.aethereon.lunareq
    PLUGIN_MANUFACTURER_CODE Aeth
    PLUGIN_CODE Lneq
    FORMATS VST3
    PRODUCT_NAME "LUNAR EQ"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Fx EQ
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(LunarEQ PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/GraphComponent.cpp
    Source/BandPanel.cpp)

target_compile_definitions(LunarEQ PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

# Tildes correctas (UTF-8)
target_compile_options(LunarEQ PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

# Fuentes incrustadas: solo si existen los 5 archivos en la carpeta fonts/
set(LUNAR_FONT_FILES
    fonts/Jost-Light.ttf
    fonts/Jost-Regular.ttf
    fonts/Jost-Medium.ttf
    fonts/Inter_18pt-Regular.ttf
    fonts/Inter_18pt-Medium.ttf)
set(LUNAR_FONTS_OK TRUE)
foreach(f ${LUNAR_FONT_FILES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
        set(LUNAR_FONTS_OK FALSE)
    endif()
endforeach()

if(LUNAR_FONTS_OK)
    juce_add_binary_data(LunarFonts SOURCES ${LUNAR_FONT_FILES})
    target_link_libraries(LunarEQ PRIVATE LunarFonts)
    target_compile_definitions(LunarEQ PUBLIC LUNAR_HAS_FONTS=1)
    message(STATUS "LUNAR EQ: fuentes incrustadas")
else()
    message(STATUS "LUNAR EQ: no se encontraron las fuentes en fonts/, se usa la fuente del sistema")
endif()

target_link_libraries(LunarEQ
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags)
