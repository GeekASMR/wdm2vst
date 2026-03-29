cmake_minimum_required(VERSION 3.20)
project(ASMRTOP_AUDIO_PLUGINS VERSION 3.2.1)

add_subdirectory(JUCE)

# ============================================================================
# Brand configuration  (set via set_brand.ps1 or manually here)
# ============================================================================
# Read brand from PluginBranding.h
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Source/PluginBranding.h" BRANDING_CONTENT)
string(FIND "${BRANDING_CONTENT}" "//#define ASMRTOP_PLUGIN_BRAND" _COMMENTED_POS)
if(_COMMENTED_POS GREATER_EQUAL 0)
    # Found //#define -> public brand (commented out)
    set(BRAND_COMPANY    "VirtualAudioRouter")
    set(BRAND_W2V_NAME   "WDM2VST")
    set(BRAND_V2W_NAME   "VST2WDM")
    set(BRAND_SEND_NAME  "Audio Send")
    set(BRAND_RECV_NAME  "Audio Receive")
    set(BRAND_MFR_CODE   "VaRt")
    set(BRAND_W2V_CODE   "w2vP")
    set(BRAND_V2W_CODE   "v2wP")
    set(BRAND_SND_CODE   "sndP")
    set(BRAND_RCV_CODE   "rcvP")
    set(BRAND_INST_W2V_NAME  "INST WDM2VST")
    set(BRAND_INST_W2V_CODE  "w2iP")
    message(STATUS "Building with PUBLIC branding")
else()
    # Not commented out -> ASMRTOP brand
    set(BRAND_COMPANY    "ASMRTOP")
    set(BRAND_W2V_NAME   "ASMRTOP WDM2VST")
    set(BRAND_V2W_NAME   "ASMRTOP VST2WDM")
    set(BRAND_SEND_NAME  "ASMRTOP Send")
    set(BRAND_RECV_NAME  "ASMRTOP Receive")
    set(BRAND_MFR_CODE   "ASMR")
    set(BRAND_W2V_CODE   "w2v1")
    set(BRAND_V2W_CODE   "v2w1")
    set(BRAND_SND_CODE   "snd1")
    set(BRAND_RCV_CODE   "rcv1")
    set(BRAND_INST_W2V_NAME  "ASMRTOP INST WDM2VST")
    set(BRAND_INST_W2V_CODE  "w2i1")
    message(STATUS "Building with ASMRTOP branding")
endif()

# ============================================================================
# WDM2VST Plugin
# ============================================================================
juce_add_plugin(ASMRTOP_WDM2VST
    COMPANY_NAME "${BRAND_COMPANY}"
    PLUGIN_MANUFACTURER_CODE "${BRAND_MFR_CODE}"
    PLUGIN_CODE "${BRAND_W2V_CODE}"
    FORMATS VST3 Standalone
    PRODUCT_NAME "${BRAND_W2V_NAME}"
    VERSION "3.2.4"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
)

target_sources(ASMRTOP_WDM2VST PRIVATE
    Source/W2V_PluginProcessor.cpp
    Source/W2V_PluginProcessor.h
    Source/W2V_PluginEditor.cpp
    Source/W2V_PluginEditor.h
)
target_compile_definitions(ASMRTOP_WDM2VST PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=1
    JUCE_VST3_CAN_REPLACE_VST2=0
)

# ============================================================================
# 1b. ASMRTOP_INST_WDM2VST (Receiver / From VAD to DAW - Instrument/Synth mode)
# ============================================================================
juce_add_plugin(ASMRTOP_INST_WDM2VST
    COMPANY_NAME "${BRAND_COMPANY}"
    IS_VST2 FALSE
    PLUGIN_MANUFACTURER_CODE "${BRAND_MFR_CODE}"
    PLUGIN_CODE "${BRAND_INST_W2V_CODE}"
    FORMATS VST3 Standalone
    PRODUCT_NAME "${BRAND_INST_W2V_NAME}"
    VERSION "3.2.4"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_NEEDS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(ASMRTOP_INST_WDM2VST PRIVATE
    Source/W2V_PluginProcessor.cpp
    Source/W2V_PluginProcessor.h
    Source/W2V_PluginEditor.cpp
    Source/W2V_PluginEditor.h
)
target_compile_definitions(ASMRTOP_INST_WDM2VST PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=1
    JUCE_VST3_CAN_REPLACE_VST2=0
    IS_INSTRUMENT_MODE=1
)
target_compile_options(ASMRTOP_INST_WDM2VST PRIVATE /utf-8)
juce_generate_juce_header(ASMRTOP_INST_WDM2VST)
target_link_libraries(ASMRTOP_INST_WDM2VST PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
)

target_compile_options(ASMRTOP_WDM2VST PRIVATE /utf-8)

juce_generate_juce_header(ASMRTOP_WDM2VST)

target_link_libraries(ASMRTOP_WDM2VST PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
)

# ============================================================================
# VST2WDM Plugin
# ============================================================================
juce_add_plugin(ASMRTOP_VST2WDM
    COMPANY_NAME "${BRAND_COMPANY}"
    PLUGIN_MANUFACTURER_CODE "${BRAND_MFR_CODE}"
    PLUGIN_CODE "${BRAND_V2W_CODE}"
    FORMATS VST3 Standalone
    PRODUCT_NAME "${BRAND_V2W_NAME}"
    VERSION "3.2.4"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
)

target_sources(ASMRTOP_VST2WDM PRIVATE
    Source/V2W_PluginProcessor.cpp
    Source/V2W_PluginProcessor.h
    Source/V2W_PluginEditor.cpp
    Source/V2W_PluginEditor.h
)
target_compile_definitions(ASMRTOP_VST2WDM PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=1
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_compile_options(ASMRTOP_VST2WDM PRIVATE /utf-8)

juce_generate_juce_header(ASMRTOP_VST2WDM)

target_link_libraries(ASMRTOP_VST2WDM PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
)

# ============================================================================
# ASMRTOP Send (P2P Audio Sender)
# ============================================================================
juce_add_plugin(ASMRTOP_SEND
    COMPANY_NAME "${BRAND_COMPANY}"
    PLUGIN_MANUFACTURER_CODE "${BRAND_MFR_CODE}"
    PLUGIN_CODE "${BRAND_SND_CODE}"
    FORMATS VST3 Standalone
    PRODUCT_NAME "${BRAND_SEND_NAME}"
    VERSION "3.2.4"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
)

target_sources(ASMRTOP_SEND PRIVATE
    Source/Send_PluginProcessor.cpp
    Source/Send_PluginEditor.cpp
)

target_compile_definitions(ASMRTOP_SEND PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=1
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_compile_options(ASMRTOP_SEND PRIVATE /utf-8)

juce_generate_juce_header(ASMRTOP_SEND)

target_link_libraries(ASMRTOP_SEND PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
)

# ============================================================================
# ASMRTOP Receive (P2P Audio Receiver)
# ============================================================================
juce_add_plugin(ASMRTOP_RECV
    COMPANY_NAME "${BRAND_COMPANY}"
    PLUGIN_MANUFACTURER_CODE "${BRAND_MFR_CODE}"
    PLUGIN_CODE "${BRAND_RCV_CODE}"
    FORMATS VST3 Standalone
    PRODUCT_NAME "${BRAND_RECV_NAME}"
    VERSION "3.2.4"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
)

target_sources(ASMRTOP_RECV PRIVATE
    Source/Recv_PluginProcessor.cpp
    Source/Recv_PluginEditor.cpp
)

target_compile_definitions(ASMRTOP_RECV PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=1
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_compile_options(ASMRTOP_RECV PRIVATE /utf-8)

juce_generate_juce_header(ASMRTOP_RECV)

target_link_libraries(ASMRTOP_RECV PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
)






