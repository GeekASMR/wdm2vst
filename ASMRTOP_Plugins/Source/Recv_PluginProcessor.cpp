#include "Recv_PluginProcessor.h"
#include "Recv_PluginEditor.h"

AsmrtopRecvAudioProcessor::AsmrtopRecvAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

AsmrtopRecvAudioProcessor::~AsmrtopRecvAudioProcessor() { stopReceiving(); }

void AsmrtopRecvAudioProcessor::prepareToPlay(double sampleRate, int) {
    currentSampleRate = sampleRate;
}
void AsmrtopRecvAudioProcessor::releaseResources() {}

void AsmrtopRecvAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    int numSamples = buffer.getNumSamples();
    float* left  = buffer.getWritePointer(0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    if (receiver.isActive()) {
        receiver.readAudio(left, right, numSamples);
    } else {
        buffer.clear();
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
}

juce::AudioProcessorEditor* AsmrtopRecvAudioProcessor::createEditor() {
    return new AsmrtopRecvAudioProcessorEditor(*this);
}

void AsmrtopRecvAudioProcessor::startReceiving() {
    receiver.start(channelId, enableIPC, enableUDP, udpPort);
}

void AsmrtopRecvAudioProcessor::stopReceiving() {
    receiver.stop();
}

void AsmrtopRecvAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto xml = std::make_unique<juce::XmlElement>("ASMRTOP_RECV");
    xml->setAttribute("channelId", channelId);
    xml->setAttribute("enableIPC", enableIPC);
    xml->setAttribute("enableUDP", enableUDP);
    xml->setAttribute("udpPort", udpPort);
    xml->setAttribute("wasReceiving", receiver.isActive());
    copyXmlToBinary(*xml, destData);
}

void AsmrtopRecvAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml && xml->hasTagName("ASMRTOP_RECV")) {
        channelId  = xml->getIntAttribute("channelId", 0);
        enableIPC  = xml->getBoolAttribute("enableIPC", true);
        enableUDP  = xml->getBoolAttribute("enableUDP", false);
        udpPort    = xml->getIntAttribute("udpPort", 18810);
        if (xml->getBoolAttribute("wasReceiving", false)) {
            startReceiving();
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new AsmrtopRecvAudioProcessor();
}
