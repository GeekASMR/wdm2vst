#include "W2V_PluginProcessor.h"
#include "W2V_PluginEditor.h"
#include "AudioUtils.h"
#include "TelemetryReporter.h"
using Asmrtop::hermite_interp;
AsmrtopWdm2VstAudioProcessor::AsmrtopWdm2VstAudioProcessor()
     : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       vts (*this, nullptr, "Parameters", { 
           std::make_unique<juce::AudioParameterFloat> ("gain", "Volume", 0.0f, 2.0f, 1.0f),
           std::make_unique<juce::AudioParameterBool> ("limiter", "Limiter", false),
           std::make_unique<juce::AudioParameterChoice> ("latencyMode", "Latency Focus", juce::StringArray{"Safe (8ms+)", "Fast (4ms+)", "Extreme (2ms+)", "Insane (1ms+ PRIO)"}, 1)
       })
{
    TelemetryReporter::getInstance().logEvent("Plugin_Opened", "Plugin Instance Created", "WDM2VST");
    deviceManager.setCurrentAudioDeviceType ("Windows Audio", true);

    deviceManager.initialise (2, 0, nullptr, false);
    deviceManager.addAudioCallback (this);
    
    juce::String defaultName = Asmrtop::SharedMemoryBridge::getIpcChannelName("PLAY", 0);
    enableIPCMode(0, defaultName + " [IPC]");
    deviceManager.closeAudioDevice(); // PREVENT DUAL AUDIO!
    
    startTimer(300);
}

AsmrtopWdm2VstAudioProcessor::~AsmrtopWdm2VstAudioProcessor() { 
    stopTimer();
    deviceManager.removeAudioCallback (this); 
}
void AsmrtopWdm2VstAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) 
{ 
    TelemetryReporter::getInstance().logEvent("DAW_Prepare", "SR: " + juce::String(sampleRate) + " | Block: " + juce::String(samplesPerBlock), "WDM2VST");
    currentDawRate = sampleRate;
    
    std::fill(ringL.begin(), ringL.end(), 0.0f);
    std::fill(ringR.begin(), ringR.end(), 0.0f);
    writePos.store(0, std::memory_order_relaxed);
    
    if (ipcBridge != nullptr && ipcBridge->isConnected() && ipcBridge->getBuffer() != nullptr) {
        uint32_t wp = ipcBridge->getBuffer()->writePos.load(std::memory_order_relaxed);
        readPos.store(wp, std::memory_order_relaxed);
    } else {
        readPos.store(0, std::memory_order_relaxed);
    }
    
    state.store(0, std::memory_order_relaxed);
    readPosFractional = 0.0;
    smoothedDiff = 0.0;
    fadeVol = 0.0f;
}
void AsmrtopWdm2VstAudioProcessor::releaseResources() {}
void AsmrtopWdm2VstAudioProcessor::enableIPCMode(int channelId, const juce::String& deviceName)
{
    isSwitching.store(true, std::memory_order_release);
    juce::Thread::sleep(5); // Wait extremely briefly for processBlock loop to exit

    ipcBridge = std::make_unique<Asmrtop::SharedMemoryBridge>(PLUGIN_IPC_BASE_NAME, "PLAY", channelId);
    currentDeviceName = deviceName;
    ipcChannelId = channelId;
    
    TelemetryReporter::getInstance().logEvent("Channel_Switch", "Target ID: " + juce::String(channelId) + " | Alias: " + deviceName, "WDM2VST");
    
    // Reset reader states so we cleanly sync to the new channel without massive diffs
    state.store(0, std::memory_order_relaxed);
    fadeVol = 0.0f;
    if (ipcBridge && ipcBridge->getBuffer()) {
        auto* buf = ipcBridge->getBuffer();
        uint32_t wp = buf->writePos.load(std::memory_order_relaxed);
        
        // CRITICAL FIX: Zero out the ring buffer region behind the write position.
        // When switching channels, the shared memory may contain stale audio data
        // from whatever was previously playing on this channel. Without clearing,
        // the fade-in will replay that old audio as residual sound.
        // Clear 4096 samples (~85ms at 48kHz) which covers the entire buffering window.
        for (int i = 0; i < 4096; ++i) {
            uint32_t idx = (wp - i) & Asmrtop::IPC_RING_MASK;
            buf->ringL[idx] = 0.0f;
            buf->ringR[idx] = 0.0f;
        }
        
        readPos.store(wp, std::memory_order_relaxed);
    } else {
        readPos.store(0, std::memory_order_relaxed);
    }
    readPosFractional = 0.0;
    smoothedDiff = 0.0;
    
    // Allow processBlock to resume safely
    isSwitching.store(false, std::memory_order_release);
}

void AsmrtopWdm2VstAudioProcessor::disableIPCMode()
{
    isSwitching.store(true, std::memory_order_release);
    juce::Thread::sleep(5);

    ipcBridge.reset();
    currentDeviceName = "";

    isSwitching.store(false, std::memory_order_release);
}



void AsmrtopWdm2VstAudioProcessor::timerCallback()
{
    if (ipcBridge != nullptr && !ipcBridge->isConnected()) {
        isSwitching.store(true, std::memory_order_release);
        ipcBridge->connect();
        if (ipcBridge->isConnected()) {
            TelemetryReporter::getInstance().logEvent("IPC_Connected", "Successfully mapped WDM shared memory for " + currentDeviceName, "WDM2VST");
            if (ipcBridge->getBuffer() != nullptr) {
                auto* buf = ipcBridge->getBuffer();
                uint32_t wp = buf->writePos.load(std::memory_order_relaxed);
                
                // Clear stale audio from ring buffer on reconnect (same fix as enableIPCMode)
                for (int i = 0; i < 4096; ++i) {
                    uint32_t idx = (wp - i) & Asmrtop::IPC_RING_MASK;
                    buf->ringL[idx] = 0.0f;
                    buf->ringR[idx] = 0.0f;
                }
                
                readPos.store(wp, std::memory_order_relaxed);
            }
            state.store(0, std::memory_order_relaxed);
            readPosFractional = 0.0;
            smoothedDiff = 0.0;
            fadeVol = 0.0f;
        }
        isSwitching.store(false, std::memory_order_release);
    }
}

void AsmrtopWdm2VstAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    int numSamples = buffer.getNumSamples();
    lastNumSamples.store(numSamples, std::memory_order_relaxed);
    
    // CRITICAL FOR LUNA / VST3: 
    // If the input track is completely silent, the VST3 host sets a 'silence' flag. 
    // JUCE wrapper propagates this silence flag to the output unless we explicitly call clear()
    // or tell it we rendered audio. Calling buffer.clear() explicitly tells JUCE 
    // that this buffer is no longer carrying the original silence footprint.
    buffer.clear();
    
    if (isSwitching.load(std::memory_order_acquire))
        return;

    
    uint32_t w = 0;
    const float* srcL = ringL.data();
    const float* srcR = ringR.data();
    bool isIPC = (ipcBridge != nullptr && ipcBridge->isConnected() && ipcBridge->getBuffer() != nullptr);

    if (isIPC) {
        w = ipcBridge->getBuffer()->writePos.load(std::memory_order_acquire);
        srcL = ipcBridge->getBuffer()->ringL;
        srcR = ipcBridge->getBuffer()->ringR;
    } else {
        w = writePos.load(std::memory_order_acquire);
    }
    
    uint32_t r = readPos.load(std::memory_order_relaxed);
    
    int32_t availableU = (int32_t)(w - r);
    
    double devRate = deviceSampleRate.load(std::memory_order_relaxed);
    if (devRate < 8000.0) devRate = 48000.0;
    double myDawRate = currentDawRate > 0.0 ? currentDawRate : 48000.0;
    double baseSpeed = devRate / myDawRate;
    int mode = (int)*vts.getRawParameterValue("latencyMode");
    
    double baseLatency = isIPC ? 0.004 : 0.040;
    int ipcExtraPadding = 32;
    double lockBounds = 0.35;
    
    if (isIPC) {
        if (mode == 0) { // Safe
            baseLatency = 0.024;
            ipcExtraPadding = 256;
            lockBounds = 0.45;
        } else if (mode == 1) { // Fast
            baseLatency = 0.016;
            ipcExtraPadding = 128;
            lockBounds = 0.35;
        } else if (mode == 2) { // Extreme
            baseLatency = 0.010;
            ipcExtraPadding = 64;
            lockBounds = 0.20;
        } else if (mode == 3) { // Insane
            baseLatency = 0.005;
            ipcExtraPadding = 32;
            lockBounds = 0.10;
        }
    }
    

    int32_t minRequired = (int32_t)(numSamples * baseSpeed) + 8;
    
    // In IPC mode we can be more aggressive with the math
    double ipcExpected = std::max((double)(devRate * baseLatency), (double)(minRequired + ipcExtraPadding));
    double wasapiExpected = std::max((double)(devRate * baseLatency), (double)(numSamples * baseSpeed * 2.5));
    double expectedDiff = isIPC ? ipcExpected : wasapiExpected;

    int32_t maxSlip = (int32_t)(expectedDiff + 4800.0);
    if (maxSlip < minRequired * 4) maxSlip = minRequired * 4;

    // Safety check for desync or extreme underrun/overrun
    if (availableU > maxSlip) {
        TelemetryReporter::getInstance().logEvent("Buffer_Overrun", "Avail: " + juce::String(availableU) + " | Expected: " + juce::String(expectedDiff, 1) + " | Req: " + juce::String(minRequired) + " | Block: " + juce::String(numSamples), "WDM2VST");
        state.store(0, std::memory_order_relaxed);
        r = w - (uint32_t)expectedDiff; 
        readPos.store(r, std::memory_order_relaxed);
        readPosFractional = 0.0;
        smoothedDiff = 0.0;
        availableU = (int32_t)expectedDiff;
        fadeVol = 0.0f;
    } else if (availableU < 0) {
        if (availableU < -1000) {
            TelemetryReporter::getInstance().logEvent("Stream_Restart", "WDM pointer reset | Avail: " + juce::String(availableU), "WDM2VST");
        } else {
            TelemetryReporter::getInstance().logEvent("Buffer_Underrun", "Avail: " + juce::String(availableU) + " | Expected: " + juce::String(expectedDiff, 1) + " | Req: " + juce::String(minRequired) + " | Block: " + juce::String(numSamples), "WDM2VST");
        }
        state.store(0, std::memory_order_relaxed);
        r = w; 
        readPos.store(r, std::memory_order_relaxed);
        readPosFractional = 0.0;
        smoothedDiff = 0.0;
        availableU = 0;
        fadeVol = 0.0f;
    }
    
    bool isPlaying = (state.load(std::memory_order_relaxed) == 1);
    if (!isPlaying && availableU >= (int32_t)expectedDiff) { 
        state.store(1, std::memory_order_relaxed);
        isPlaying = true;
    } else if (isPlaying && availableU < (int32_t)(minRequired / 2)) { // Only stop playing if severely drained (half of minRequired)
        state.store(0, std::memory_order_relaxed);
        isPlaying = false;
    }
    
    // If not playing and completely faded out, just output silence and don't advance read pointer
    if (!isPlaying && fadeVol < 0.0001f) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* out = buffer.getWritePointer(ch);
            juce::FloatVectorOperations::clear(out, numSamples);
        }
        return;
    }

    double read_speed = baseSpeed;
    if (isPlaying) {
        double rawDiff = (double)availableU - expectedDiff;
        // Make smoothing slower to ignore OS block size jitter
        smoothedDiff = smoothedDiff * 0.995 + rawDiff * 0.005; 
        
        // Jitter Lock:
        // Use user-configurable bounds to give variable breathing room!
        if (std::abs(smoothedDiff) < expectedDiff * lockBounds && isIPC) {
            read_speed = baseSpeed;
            // Slowly drift fractional part to 0.0 so we read bit-perfect samples
            readPosFractional *= 0.99;
        } else {
            read_speed = baseSpeed + (smoothedDiff * 0.0001); // Stronger push 0.0001 to handle 48k->44k seamlessly
        }
        
        if (read_speed > baseSpeed * 1.50) read_speed = baseSpeed * 1.50; // Allow true resampling headroom
        if (read_speed < baseSpeed * 0.50) read_speed = baseSpeed * 0.50;
    }
    
    float* outL = buffer.getWritePointer(0);
    float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
    float gain = *vts.getRawParameterValue("gain");
    bool limitOn = *vts.getRawParameterValue("limiter") > 0.5f;
    
    float fadeTarget = isPlaying ? 1.0f : 0.0f;
    
    for (int i = 0; i < numSamples; ++i) {
        fadeVol += (fadeTarget - fadeVol) * 0.005f;
        
        float f = (float)readPosFractional;
        uint32_t rm1 = (r - 1) & Asmrtop::IPC_RING_MASK;
        uint32_t r0  = r & Asmrtop::IPC_RING_MASK;
        uint32_t r1  = (r + 1) & Asmrtop::IPC_RING_MASK;
        uint32_t r2  = (r + 2) & Asmrtop::IPC_RING_MASK;
        if (outL) {
            float sample = hermite_interp(srcL[rm1], srcL[r0], srcL[r1], srcL[r2], f) * gain * fadeVol;
            if (std::isnan(sample) || std::isinf(sample)) sample = 0.0f;
            if (limitOn) { if (sample > 0.99f) sample = 0.99f; else if (sample < -0.99f) sample = -0.99f; }
            outL[i] = sample;
        }
        if (outR) {
            float sample = hermite_interp(srcR[rm1], srcR[r0], srcR[r1], srcR[r2], f) * gain * fadeVol;
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
                fadeVol *= 0.9f; // Soft emergency fade (0.9f instead of 0.5f to prevent clicking/current sound)
            }
        }
    }
    
    readPos.store(r, std::memory_order_release);
    
    // Peak meter calculations
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

void AsmrtopWdm2VstAudioProcessor::audioDeviceIOCallbackWithContext (const float* const* inputChannelData, int numInputChannels, float* const* outputChannelData, int numOutputChannels, int numSamples, const juce::AudioIODeviceCallbackContext& context)
{
    uint32_t w = writePos.load(std::memory_order_relaxed);
    
    const float* inL = numInputChannels > 0 ? inputChannelData[0] : nullptr;
    const float* inR = numInputChannels > 1 ? inputChannelData[1] : nullptr;
    
    for (int i = 0; i < numSamples; ++i) 
    {
        ringL[(w + i) & Asmrtop::IPC_RING_MASK] = inL ? inL[i] : 0.0f;
        ringR[(w + i) & Asmrtop::IPC_RING_MASK] = inR ? inR[i] : 0.0f;
    }
    
    // uint32_t subtraction naturally handles wrap-around
    writePos.store(w + numSamples, std::memory_order_release);
}

void AsmrtopWdm2VstAudioProcessor::audioDeviceAboutToStart (juce::AudioIODevice* device) 
{ 
    if (device != nullptr) deviceSampleRate.store(device->getCurrentSampleRate(), std::memory_order_relaxed);
    std::fill(ringL.begin(), ringL.end(), 0.0f);
    std::fill(ringR.begin(), ringR.end(), 0.0f);
    writePos.store(0, std::memory_order_relaxed);
    readPos.store(0, std::memory_order_relaxed);
    state.store(0, std::memory_order_relaxed);
    readPosFractional = 0.0;
    smoothedDiff = 0.0;
}

void AsmrtopWdm2VstAudioProcessor::audioDeviceStopped() {}

juce::AudioProcessorEditor* AsmrtopWdm2VstAudioProcessor::createEditor() { return new AsmrtopWdm2VstAudioProcessorEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AsmrtopWdm2VstAudioProcessor();
}

juce::String AsmrtopWdm2VstAudioProcessor::getDeviceDiagnostics() {
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

int AsmrtopWdm2VstAudioProcessor::getAvailableSamples() const {
    if (isSwitching.load(std::memory_order_acquire)) return 0;
    if (ipcBridge != nullptr && ipcBridge->isConnected() && ipcBridge->getBuffer() != nullptr) {
        uint32_t w = ipcBridge->getBuffer()->writePos.load(std::memory_order_relaxed);
        uint32_t r = readPos.load(std::memory_order_relaxed);
        int32_t avail = (int32_t)(w - r);
        return (avail < 0 || avail > Asmrtop::IPC_RING_SIZE) ? 0 : avail;
    }
    return (int)(writePos.load(std::memory_order_relaxed) - readPos.load(std::memory_order_relaxed));
}

double AsmrtopWdm2VstAudioProcessor::getLatencyMs() const {
    double sr = deviceSampleRate.load(std::memory_order_relaxed);
    if (sr < 8000.0) sr = 48000.0;
    return (double)getAvailableSamples() / sr * 1000.0;
}
