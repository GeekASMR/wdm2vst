#pragma once
#include <JuceHeader.h>
#include "W2V_PluginProcessor.h"
#include "Localization.h"
#include "DropdownLookAndFeel.h"

class AsmrtopWdm2VstAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::ComboBox::Listener, public juce::Timer
{
public:
    AsmrtopWdm2VstAudioProcessorEditor (AsmrtopWdm2VstAudioProcessor&);
    ~AsmrtopWdm2VstAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void timerCallback() override;
private:
    AsmrtopWdm2VstAudioProcessor& audioProcessor;
    juce::ComboBox deviceComboBox;
    juce::StringArray inputDeviceNames;
    juce::Slider volumeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;
    juce::ToggleButton limiterToggle;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> limiterAttachment;
    juce::ComboBox latencyModeComboBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> latencyModeAttachment;
    juce::TextButton langBtn;
    DropdownLookAndFeel dropdownLnf;
    float displayedPeakL { 0.0f };
    float displayedPeakR { 0.0f };
    void refreshDeviceList();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AsmrtopWdm2VstAudioProcessorEditor)
};
