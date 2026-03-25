#pragma once
#include <JuceHeader.h>
#include "Recv_PluginProcessor.h"
#include "Localization.h"

class AsmrtopRecvAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    AsmrtopRecvAudioProcessorEditor(AsmrtopRecvAudioProcessor&);
    ~AsmrtopRecvAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    AsmrtopRecvAudioProcessor& audioProcessor;

    juce::ComboBox channelSelector;
    juce::ToggleButton ipcToggle;
    juce::ToggleButton udpToggle;
    juce::TextEditor   udpPortEditor;
    juce::TextButton   startStopBtn;
    juce::TextButton   langBtn;

    float displayPeakL = 0.0f, displayPeakR = 0.0f;

    void updateStartStopButton();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AsmrtopRecvAudioProcessorEditor)
};
