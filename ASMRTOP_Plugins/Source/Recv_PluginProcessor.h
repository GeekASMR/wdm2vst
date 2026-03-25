#pragma once
#include <JuceHeader.h>
#include "P2PTransport.h"
#include "PluginBranding.h"

class AsmrtopRecvAudioProcessor : public juce::AudioProcessor
{
public:
    AsmrtopRecvAudioProcessor();
    ~AsmrtopRecvAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return PLUGIN_COMPANY_NAME " Receive"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 99999.0; }
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override {
        return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
    }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- P2P Controls ---
    void startReceiving();
    void stopReceiving();
    bool isReceiving() const { return receiver.isActive(); }

    // Settings
    int channelId = 0;
    bool enableIPC = true;
    bool enableUDP = false;
    int udpPort = 18810;

    // Metering
    std::atomic<float> peakL { 0.0f };
    std::atomic<float> peakR { 0.0f };

    Asmrtop::P2PReceiver& getReceiver() { return receiver; }

private:
    Asmrtop::P2PReceiver receiver;
    double currentSampleRate = 48000.0;
    std::unique_ptr<Asmrtop::SharedMemoryBridge> ipcBridge;
    juce::String currentDeviceName;
    int ipcChannelId { 0 };
    std::atomic<bool> isSwitching { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AsmrtopRecvAudioProcessor)
};
