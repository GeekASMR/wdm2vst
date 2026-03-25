#pragma once
#include <JuceHeader.h>
#include "AsmrtopIPC.h"
#include "PluginBranding.h"
#include "P2PTransport.h"

class AsmrtopVst2WdmAudioProcessor : public juce::AudioProcessor, public juce::AudioIODeviceCallback
{
public:
    AsmrtopVst2WdmAudioProcessor();
    ~AsmrtopVst2WdmAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return PLUGIN_V2W_NAME; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 99999.0; }
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override { 
        if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
            return false;
        return true;
    }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return {}; }
    void changeProgramName (int index, const juce::String& newName) override {}
    void getStateInformation (juce::MemoryBlock& destData) override {
        juce::ValueTree stateTree = vts.copyState();
        std::unique_ptr<juce::XmlElement> xml (stateTree.createXml());
        if (auto devSetup = deviceManager.createStateXml()) xml->addChildElement(devSetup.release());
        if (ipcBridge != nullptr) {
            xml->setAttribute("ipcName", currentDeviceName);
            xml->setAttribute("ipcChannelId", ipcChannelId);
        }
        copyXmlToBinary (*xml, destData);
    }
    void setStateInformation (const void* data, int sizeInBytes) override {
        std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
        if (xmlState != nullptr && xmlState->hasTagName (vts.state.getType())) {
            vts.replaceState (juce::ValueTree::fromXml (*xmlState));
            if (auto* devSetup = xmlState->getChildByName("DEVICESETUP")) deviceManager.initialise(0, 2, devSetup, false);
            if (xmlState->hasAttribute("ipcChannelId")) {
                int chId = xmlState->getIntAttribute("ipcChannelId", 0);
                juce::String name = xmlState->getStringAttribute("ipcName", "IPC " + juce::String(chId));
                enableIPCMode(chId, name);
            }
        }
    }

    void audioDeviceIOCallbackWithContext (const float* const* inputChannelData, int numInputChannels,
                                           float* const* outputChannelData, int numOutputChannels, int numSamples,
                                           const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart (juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    juce::AudioDeviceManager deviceManager;
    juce::AudioProcessorValueTreeState vts;
    std::atomic<float> currentPeakLevelLeft { 0.0f };
    std::atomic<float> currentPeakLevelRight { 0.0f };
    juce::String getDeviceDiagnostics();
    int getAvailableSamples() const;
    double getLatencyMs() const;

    void enableIPCMode(int channelId, const juce::String& deviceName);
    void disableIPCMode();
    bool isIPCMode() const { return ipcBridge != nullptr; }

    juce::String getCurrentDeviceName() const { return currentDeviceName; }

    // Mobile Monitor
    bool enableMobile = false;
    int wsPort = 18820;
    Asmrtop::P2PSender mobileSender;
    void enableMobileMonitor() { mobileSender.start(0, (int)currentDawRate, false, false, "", 0, true, wsPort); }
    void disableMobileMonitor() { mobileSender.stop(); }
    bool isMobileActive() const { return mobileSender.isWSRunning(); }

private:
    std::unique_ptr<Asmrtop::SharedMemoryBridge> ipcBridge;
    juce::String currentDeviceName;
    int ipcChannelId { 0 };

    std::vector<float> ringL { std::vector<float>(Asmrtop::IPC_RING_SIZE, 0.0f) };
    std::vector<float> ringR { std::vector<float>(Asmrtop::IPC_RING_SIZE, 0.0f) };
    std::atomic<uint32_t> writePos { 0 };
    std::atomic<uint32_t> readPos { 0 };
    std::atomic<int> state { 0 }; // 0 = Buffering, 1 = Playing
    double readPosFractional { 0.0 };
    double smoothedDiff { 0.0 };
    float fadeVol { 0.0f };
    double currentDawRate = 48000.0;
    std::atomic<double> deviceSampleRate { 48000.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AsmrtopVst2WdmAudioProcessor)
};
