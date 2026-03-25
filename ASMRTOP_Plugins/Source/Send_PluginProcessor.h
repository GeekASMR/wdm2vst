#pragma once
#include <JuceHeader.h>
#include "P2PTransport.h"
#include "PluginBranding.h"

class AsmrtopSendAudioProcessor : public juce::AudioProcessor
{
public:
    AsmrtopSendAudioProcessor();
    ~AsmrtopSendAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return PLUGIN_COMPANY_NAME " Send"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override {
        return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
            && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- P2P Controls ---
    void startSending();
    void stopSending();
    bool isSending() const { return sender.isActive(); }

    // Settings
    int channelId = 0;
    bool enableIPC = true;
    bool enableUDP = false;
    bool enableWS = false;
    juce::String udpTargetIP { "127.0.0.1" };
    int udpPort = 18810;
    int wsPort = 18820;

    // Metering
    std::atomic<float> peakL { 0.0f };
    std::atomic<float> peakR { 0.0f };

    Asmrtop::P2PSender& getSender() { return sender; }

private:
    Asmrtop::P2PSender sender;
    double currentSampleRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AsmrtopSendAudioProcessor)
};
