#pragma once
#include <JuceHeader.h>
#include "V2W_PluginProcessor.h"
#include "Localization.h"
#include "QRCodePopup.h"
#include "DropdownLookAndFeel.h"

class AsmrtopVst2WdmAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::ComboBox::Listener, public juce::Timer
{
public:
    AsmrtopVst2WdmAudioProcessorEditor (AsmrtopVst2WdmAudioProcessor&);
    ~AsmrtopVst2WdmAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void timerCallback() override;
private:
    AsmrtopVst2WdmAudioProcessor& audioProcessor;
    juce::ComboBox deviceComboBox;
    juce::StringArray outputDeviceNames;
    juce::StringArray allInputNames;
    juce::Slider volumeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;
    juce::ToggleButton limiterToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> limiterAttachment;

    // Mobile Monitor (Plan A)
    juce::ToggleButton mobileToggle;
    juce::TextEditor wsPortEditor;
    juce::TextButton qrBtn;
    juce::TextButton firewallBtn;
    bool portOpen = false;

    // Language
    juce::TextButton langBtn;
    DropdownLookAndFeel dropdownLnf;

    float displayedPeakL { 0.0f };
    float displayedPeakR { 0.0f };

    void showQRCode();
    void openFirewallPort(int port);
    void refreshDeviceList();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AsmrtopVst2WdmAudioProcessorEditor)
};
