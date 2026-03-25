#pragma once
#include <JuceHeader.h>
#include "Send_PluginProcessor.h"
#include "QRCodePopup.h"

class AsmrtopSendAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    AsmrtopSendAudioProcessorEditor(AsmrtopSendAudioProcessor&);
    ~AsmrtopSendAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    AsmrtopSendAudioProcessor& audioProcessor;

    juce::ComboBox channelSelector;
    juce::ToggleButton ipcToggle;
    juce::ToggleButton udpToggle;
    juce::ToggleButton wsToggle;
    juce::TextEditor   udpIPEditor;
    juce::TextEditor   udpPortEditor;
    juce::TextEditor   wsPortEditor;
    juce::TextButton   startStopBtn;
    juce::TextButton   qrBtn;
    juce::TextButton   firewallBtn;
    juce::TextButton   langBtn;

    float displayPeakL = 0.0f, displayPeakR = 0.0f;
    bool portOpen = false;

    void updateStartStopButton();
    void showQRCode();
    bool checkFirewallPort(int port);
    void openFirewallPort(int port);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AsmrtopSendAudioProcessorEditor)
};
