#include "V2W_PluginProcessor.h"
#include "V2W_PluginEditor.h"
#include "AudioUtils.h"
#include "TelemetryReporter.h"
using Asmrtop::hermite_interp;

AsmrtopVst2WdmAudioProcessor::AsmrtopVst2WdmAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       vts (*this, nullptr, "Parameters", { 
           std::make_unique<juce::AudioParameterFloat> ("gain", "Volume", 0.0f, 2.0f, 1.0f),
           std::make_unique<juce::AudioParameterBool> ("limiter", "Limiter", false)
       })
{
    TelemetryReporter::getInstance().logEvent("Plugin_Opened", "Plugin Instance Created", "VST2WDM");
    deviceManager.setCurrentAudioDeviceType ("Windows Audio", true);

    deviceManager.initialise (0, 2, nullptr, false);
    deviceManager.addAudioCallback (this);
    
    juce::String defaultName = Asmrtop::SharedMemoryBridge::getIpcChannelName("REC", 0);
    enableIPCMode(0, defaultName + " [IPC]");
    deviceManager.closeAudioDevice(); // PREVENT DUAL AUDIO!
    
    startTimer(300);
}

AsmrtopVst2WdmAudioProcessor::~AsmrtopVst2WdmAudioProcessor() { deviceManager.removeAudioCallback (this); }
void AsmrtopVst2WdmAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) 
{ 
    TelemetryReporter::getInstance().logEvent("DAW_Prepare", "SR: " + juce::String(sampleRate) + " | Block: " + juce::String(samplesPerBlock), "VST2WDM");
    currentDawRate = sampleRate;
    
    std::fill(ringL.begin(), ringL.end(), 0.0f);
    std::fill(ringR.begin(), ringR.end(), 0.0f);
    writePos.store(0, std::memory_order_relaxed);
    readPos.store(0, std::memory_order_relaxed);
    state.store(0, std::memory_order_relaxed);
    readPosFractional = 0.0;
    smoothedDiff = 0.0;
}
void AsmrtopVst2WdmAudioProcessor::releaseResources() {}
void AsmrtopVst2WdmAudioProcessor::enableIPCMode(int channelId, const juce::String& deviceName)
{
    ipcBridge = std::make_unique<Asmrtop::SharedMemoryBridge>(PLUGIN_IPC_BASE_NAME, "REC", channelId);
    currentDeviceName = deviceName;
    ipcChannelId = channelId;
}

void AsmrtopVst2WdmAudioProcessor::disableIPCMode()
{
    ipcBridge.reset();
    currentDeviceName = "";
}


void AsmrtopVst2WdmAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    int numSamples = buffer.getNumSamples();
    const float* inL = buffer.getNumChannels() > 0 ? buffer.getReadPointer(0) : nullptr;
    const float* inR = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : nullptr;
    
    float gain = *vts.getRawParameterValue("gain");
    bool limitOn = *vts.getRawParameterValue("limiter") > 0.5f;
    
    if (ipcBridge != nullptr && ipcBridge->isConnected())
    {
        // IPC mode: direct write to shared memory, no resampling needed
        for (int i = 0; i < numSamples; ++i)
        {
            float vL = (inL ? inL[i] : 0.0f) * gain;
            float vR = (inR ? inR[i] : 0.0f) * gain;
            if (limitOn) {
                if (vL > 0.99f) vL = 0.99f; else if (vL < -0.99f) vL = -0.99f;
                if (vR > 0.99f) vR = 0.99f; else if (vR < -0.99f) vR = -0.99f;
            }
            ringL[i] = vL;
            ringR[i] = vR;
        }
        ipcBridge->write(ringL.data(), ringR.data(), numSamples);
    }
    else
    {
        // WASAPI mode: write to local ring buffer for device callback
        uint32_t w = writePos.load(std::memory_order_relaxed);
        for (int i = 0; i < numSamples; ++i) 
        {
            float vL = (inL ? inL[i] : 0.0f) * gain;
            float vR = (inR ? inR[i] : 0.0f) * gain;
            if (limitOn) {
                if (vL > 0.99f) vL = 0.99f; else if (vL < -0.99f) vL = -0.99f;
                if (vR > 0.99f) vR = 0.99f; else if (vR < -0.99f) vR = -0.99f;
            }
            ringL[(w + i) & Asmrtop::IPC_RING_MASK] = vL;
            ringR[(w + i) & Asmrtop::IPC_RING_MASK] = vR;
        }
        writePos.store(w + numSamples, std::memory_order_release);
    }

    // Mobile Monitor: push to WebSocket clients
    if (mobileSender.isWSRunning()) {
        mobileSender.pushToWebSocketClients(inL, inR, numSamples);
    }

    float peakL = 0.0f;
    float peakR = 0.0f;
    if (buffer.getNumChannels() > 0)
    {
        auto mag = juce::FloatVectorOperations::findMinAndMax(buffer.getReadPointer(0), buffer.getNumSamples());
        peakL = juce::jmax(peakL, std::abs((float)mag.getStart()), std::abs((float)mag.getEnd()));
        peakL = juce::jmax(peakL, currentPeakLevelLeft.load());
    }
    if (buffer.getNumChannels() > 1)
    {
        auto mag = juce::FloatVectorOperations::findMinAndMax(buffer.getReadPointer(1), buffer.getNumSamples());
        peakR = juce::jmax(peakR, std::abs((float)mag.getStart()), std::abs((float)mag.getEnd()));
        peakR = juce::jmax(peakR, currentPeakLevelRight.load());
    }
    currentPeakLevelLeft.store(peakL);
    currentPeakLevelRight.store(peakR);
}

void AsmrtopVst2WdmAudioProcessor::audioDeviceIOCallbackWithContext (const float* const* inputChannelData, int numInputChannels, float* const* outputChannelData, int numOutputChannels, int numSamples, const juce::AudioIODeviceCallbackContext& context)
{
    for (int ch = 0; ch < numOutputChannels; ++ch)
        juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);

    if (numOutputChannels == 0) return;

    uint32_t w = writePos.load(std::memory_order_acquire);
    uint32_t r = readPos.load(std::memory_order_relaxed);
    
    int32_t availableU = (int32_t)(w - r);
    
    double devRate = deviceSampleRate.load(std::memory_order_relaxed);
    if (devRate < 8000.0) devRate = 48000.0;
    double myDawRate = currentDawRate > 0.0 ? currentDawRate : 48000.0;
    double baseSpeed = myDawRate / devRate;
    double expectedDiff = std::max((double)(myDawRate * 0.060), (double)(numSamples * baseSpeed * 2.5));

    int32_t minRequired = (int32_t)(numSamples * baseSpeed) + 8;

    // Safety check
    if (availableU > Asmrtop::IPC_RING_SIZE) {
        TelemetryReporter::getInstance().logEvent("Buffer_Overrun", "Avail: " + juce::String(availableU), "VST2WDM");
        state.store(0, std::memory_order_relaxed);
        r = w - (uint32_t)expectedDiff; 
        readPos.store(r, std::memory_order_relaxed);
        readPosFractional = 0.0;
        smoothedDiff = 0.0;
        availableU = (int32_t)expectedDiff;
    } else if (availableU < 0) {
        TelemetryReporter::getInstance().logEvent("Buffer_Underrun", "Avail: " + juce::String(availableU), "VST2WDM");
        state.store(0, std::memory_order_relaxed);
        r = w; 
        readPos.store(r, std::memory_order_relaxed);
        readPosFractional = 0.0;
        smoothedDiff = 0.0;
        availableU = 0;
    }
    
    bool isPlaying = (state.load(std::memory_order_relaxed) == 1);
    if (!isPlaying && availableU >= (int32_t)expectedDiff) { 
        state.store(1, std::memory_order_relaxed);
        isPlaying = true;
    } else if (isPlaying && availableU < minRequired) {
        state.store(0, std::memory_order_relaxed);
        isPlaying = false;
    }
    
    if (!isPlaying && fadeVol < 0.0001f) {
        return; // Output is already cleared at the top of the function
    }
    
    double read_speed = baseSpeed;
    if (isPlaying) {
        double rawDiff = (double)availableU - expectedDiff;
        smoothedDiff = smoothedDiff * 0.99 + rawDiff * 0.01;
        read_speed = baseSpeed + (smoothedDiff * 0.00002);
        if (read_speed > baseSpeed * 1.05) read_speed = baseSpeed * 1.05;
        if (read_speed < baseSpeed * 0.95) read_speed = baseSpeed * 0.95;
    }

    float gain = *vts.getRawParameterValue("gain");
    bool limitOn = *vts.getRawParameterValue("limiter") > 0.5f;
    float* outL = outputChannelData[0];
    float* outR = numOutputChannels > 1 ? outputChannelData[1] : nullptr;
    
    float fadeTarget = isPlaying ? 1.0f : 0.0f;

    for (int i = 0; i < numSamples; ++i) {
        fadeVol += (fadeTarget - fadeVol) * 0.005f;
        
        float f = (float)readPosFractional;
        uint32_t rm1 = (r - 1) & Asmrtop::IPC_RING_MASK;
        uint32_t r0  = r & Asmrtop::IPC_RING_MASK;
        uint32_t r1  = (r + 1) & Asmrtop::IPC_RING_MASK;
        uint32_t r2  = (r + 2) & Asmrtop::IPC_RING_MASK;
        if (outL) {
            float sample = hermite_interp(ringL[rm1], ringL[r0], ringL[r1], ringL[r2], f) * gain * fadeVol;
            if (std::isnan(sample) || std::isinf(sample)) sample = 0.0f;
            if (limitOn) { if (sample > 0.99f) sample = 0.99f; else if (sample < -0.99f) sample = -0.99f; }
            outL[i] = sample;
        }
        if (outR) {
            float sample = hermite_interp(ringR[rm1], ringR[r0], ringR[r1], ringR[r2], f) * gain * fadeVol;
            if (std::isnan(sample) || std::isinf(sample)) sample = 0.0f;
            if (limitOn) { if (sample > 0.99f) sample = 0.99f; else if (sample < -0.99f) sample = -0.99f; }
            outR[i] = sample;
        }
        
        if (isPlaying || fadeVol >= 0.0001f) {
            uint32_t dist = w - r;
            if ((int32_t)dist > 2) {
                readPosFractional += read_speed;
                while (readPosFractional >= 1.0) {
                    readPosFractional -= 1.0;
                    r += 1;
                }
            } else {
                fadeVol *= 0.5f; // Fast emergency fade
            }
        }
    }
    
    readPos.store(r, std::memory_order_release);
}

void AsmrtopVst2WdmAudioProcessor::audioDeviceAboutToStart (juce::AudioIODevice* device) 
{ 
    if (device != nullptr) deviceSampleRate.store(device->getCurrentSampleRate(), std::memory_order_relaxed);
    std::fill(ringL.begin(), ringL.end(), 0.0f);
    std::fill(ringR.begin(), ringR.end(), 0.0f);
    writePos.store(0, std::memory_order_relaxed);
    readPos.store(0, std::memory_order_relaxed);
    state.store(0, std::memory_order_relaxed);
    readPosFractional = 0.0;
    smoothedDiff = 0.0;
    fadeVol = 0.0f;
}

void AsmrtopVst2WdmAudioProcessor::audioDeviceStopped() {}

juce::AudioProcessorEditor* AsmrtopVst2WdmAudioProcessor::createEditor() { return new AsmrtopVst2WdmAudioProcessorEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AsmrtopVst2WdmAudioProcessor();
}

juce::String AsmrtopVst2WdmAudioProcessor::getDeviceDiagnostics() {
    if (ipcBridge != nullptr && ipcBridge->isConnected()) {
        return currentDeviceName + " | Memory Mapped | IPC Sync";
    }
    if (auto* dev = deviceManager.getCurrentAudioDevice()) {
        return dev->getName() + " | " + juce::String(dev->getCurrentSampleRate(), 0) + "Hz | In:" +
               juce::String(dev->getActiveInputChannels().countNumberOfSetBits()) + " Out:" +
               juce::String(dev->getActiveOutputChannels().countNumberOfSetBits()) + " | Buf:" +
               juce::String(dev->getCurrentBufferSizeSamples());
    }
    return "Not Connected";
}

int AsmrtopVst2WdmAudioProcessor::getAvailableSamples() const {
    if (ipcBridge != nullptr && ipcBridge->isConnected() && ipcBridge->getBuffer() != nullptr) {
        uint32_t w = ipcBridge->getBuffer()->writePos.load(std::memory_order_relaxed);
        uint32_t r = ipcBridge->getBuffer()->readPos.load(std::memory_order_relaxed);
        int32_t avail = (int32_t)(w - r);
        return (avail < 0 || avail > Asmrtop::IPC_RING_SIZE) ? 0 : avail;
    }
    return (int)(writePos.load(std::memory_order_relaxed) - readPos.load(std::memory_order_relaxed));
}

double AsmrtopVst2WdmAudioProcessor::getLatencyMs() const {
    double sr = currentDawRate > 0.0 ? currentDawRate : 48000.0;
    if (ipcBridge != nullptr) {
        return (double)getAvailableSamples() / sr * 1000.0;
    }
    sr = deviceSampleRate.load(std::memory_order_relaxed);
    if (sr < 8000.0) sr = 48000.0;
    return (double)getAvailableSamples() / sr * 1000.0;
}

void AsmrtopVst2WdmAudioProcessor::timerCallback()
{
    if (ipcBridge != nullptr && !ipcBridge->isConnected()) {
        ipcBridge->connect();
    }
}
