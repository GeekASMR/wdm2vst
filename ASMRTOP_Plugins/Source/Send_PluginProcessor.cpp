#include "Send_PluginProcessor.h"
#include "Send_PluginEditor.h"

AsmrtopSendAudioProcessor::AsmrtopSendAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

AsmrtopSendAudioProcessor::~AsmrtopSendAudioProcessor() { stopSending(); }

void AsmrtopSendAudioProcessor::prepareToPlay(double sampleRate, int) {
    currentSampleRate = sampleRate;
}
void AsmrtopSendAudioProcessor::releaseResources() {}

void AsmrtopSendAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    const float* left  = totalNumInputChannels > 0 ? buffer.getReadPointer(0) : nullptr;
    const float* right = totalNumInputChannels > 1 ? buffer.getReadPointer(1) : nullptr;
    int numSamples = buffer.getNumSamples();

    // Send to all active transports
    if (sender.isActive()) {
        sender.sendAudio(left, right, numSamples);
    }

    // B1: SIMD peak metering
    float pL = 0.0f, pR = 0.0f;
    if (left) {
        auto mag = juce::FloatVectorOperations::findMinAndMax(left, numSamples);
        pL = juce::jmax(std::abs(mag.getStart()), std::abs(mag.getEnd()));
    }
    if (right) {
        auto mag = juce::FloatVectorOperations::findMinAndMax(right, numSamples);
        pR = juce::jmax(std::abs(mag.getStart()), std::abs(mag.getEnd()));
    }
    peakL.store(pL, std::memory_order_relaxed);
    peakR.store(pR, std::memory_order_relaxed);

    // Pass-through: output = input (don't break the DAW signal chain)
    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear(ch, 0, numSamples);
}

juce::AudioProcessorEditor* AsmrtopSendAudioProcessor::createEditor() {
    return new AsmrtopSendAudioProcessorEditor(*this);
}

void AsmrtopSendAudioProcessor::startSending() {
    sender.start(channelId, (int)currentSampleRate, enableIPC, enableUDP, udpTargetIP, udpPort, enableWS, wsPort);
}

void AsmrtopSendAudioProcessor::stopSending() {
    sender.stop();
}

void AsmrtopSendAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto xml = std::make_unique<juce::XmlElement>("ASMRTOP_SEND");
    xml->setAttribute("channelId", channelId);
    xml->setAttribute("enableIPC", enableIPC);
    xml->setAttribute("enableUDP", enableUDP);
    xml->setAttribute("enableWS", enableWS);
    xml->setAttribute("udpTargetIP", udpTargetIP);
    xml->setAttribute("udpPort", udpPort);
    xml->setAttribute("wsPort", wsPort);
    xml->setAttribute("wasSending", sender.isActive());
    copyXmlToBinary(*xml, destData);
}

void AsmrtopSendAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml && xml->hasTagName("ASMRTOP_SEND")) {
        channelId   = xml->getIntAttribute("channelId", 0);
        enableIPC   = xml->getBoolAttribute("enableIPC", true);
        enableUDP   = xml->getBoolAttribute("enableUDP", false);
        enableWS    = xml->getBoolAttribute("enableWS", false);
        udpTargetIP = xml->getStringAttribute("udpTargetIP", "255.255.255.255");
        udpPort     = xml->getIntAttribute("udpPort", 18810);
        wsPort      = xml->getIntAttribute("wsPort", 18820);
        if (xml->getBoolAttribute("wasSending", false)) {
            startSending();
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AsmrtopSendAudioProcessor();
}
