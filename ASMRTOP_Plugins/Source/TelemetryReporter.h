#pragma once
#include <JuceHeader.h>
#include <queue>
#include <mutex>

class TelemetryReporter : public juce::Thread {
public:
    TelemetryReporter() : juce::Thread("WDMTelmetryWorker") {
        startThread();
    }
    
    ~TelemetryReporter() {
        signalThreadShouldExit();
        notify();
        stopThread(1500);
    }
    
    // Thread-safe Singleton
    static TelemetryReporter& getInstance() {
        static TelemetryReporter instance;
        return instance;
    }

    // Call this from ANY thread securely
    void logEvent(const juce::String& eventName, const juce::String& details, const juce::String& pluginName) {
        // Enforce user privacy settings via Setup INI
        juce::String cp = juce::SystemStats::getEnvironmentVariable("CommonProgramFiles", "C:\\Program Files\\Common Files");
        juce::File configIni (cp + "\\VST3\\VirtualAudioRouter\\config.ini");
        if (!configIni.existsAsFile() || !configIni.loadFileAsString().contains("EnableTelemetry=1")) {
            return; // User explicitly denied telemetry or file is missing
        }
        
        juce::String timestamp = juce::Time::getCurrentTime().toISO8601(true);
        
        // Escape json
        juce::String safeEvent = eventName.replace("\"", "\\\"");
        juce::String safeDetails = details.replace("\"", "\\\"");
        // Detailed host context
        juce::String osName = juce::SystemStats::getOperatingSystemName() + " (" + (juce::SystemStats::isOperatingSystem64Bit() ? "x64" : "x86") + ")";
        juce::String dName = juce::SystemStats::getComputerName();
        juce::String dawName = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFileNameWithoutExtension();
        
        // Simple JSON Payload
        juce::String payload = "{ \"event\": \"" + safeEvent + "\", " +
                               "\"details\": \"" + safeDetails + "\", " +
                               "\"plugin\": \"" + pluginName + "\", " +
                               "\"daw\": \"" + dawName + "\", " +
                               "\"os\": \"" + osName + "\", " +
                               "\"device\": \"" + dName + "\", " +
                               "\"time\": \"" + timestamp + "\" }";
                               
        std::lock_guard<std::mutex> lock(queueMutex);
        if (logQueue.size() < 200) { // Keep queue bounded to avoid memory explosion if offline
            logQueue.push(payload);
            notify();
        }
    }

private:
    void run() override {
        while (!threadShouldExit()) {
            juce::String payloadToSend;
            
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (!logQueue.empty()) {
                    payloadToSend = logQueue.front();
                    logQueue.pop();
                }
            }
            
            if (payloadToSend.isNotEmpty()) {
                // Here we use the server IP directly for the POST request endpoint
                // In a real environment, you will build a PHP or Node API interface here to receive JSON.
                juce::URL telemetryUrl("https://geek.asmrtop.cn/log_capture.php");
                
                juce::StringArray headers;
                headers.add("Content-Type: application/json");
                
                auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inPostData)
                               .withExtraHeaders(headers.joinIntoString("\r\n"))
                               .withConnectionTimeoutMs(3000)
                               .withHttpRequestCmd("POST");
                               
                juce::URL postUrl = telemetryUrl.withPOSTData(payloadToSend);
                std::unique_ptr<juce::InputStream> stream(postUrl.createInputStream(options));
                
                // Execute Network Request. We discard stream content, just need the HTTP transmission.
                if (stream != nullptr) {
                    stream->readEntireStreamAsString();
                } else {
                    // if fail, we don't hold up the queue for now. (Можно put it back to queue if we want redundancy)
                }
                
                // Wait briefly between logs to not spam
                wait(200);
            } else {
                wait(-1); // Sleep deeply until `notify()` wakes us up (meaning new event came)
            }
        }
    }
    
    std::queue<juce::String> logQueue;
    std::mutex queueMutex;
};
