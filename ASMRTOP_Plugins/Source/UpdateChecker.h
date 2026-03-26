#pragma once
#include <JuceHeader.h>

class UpdateChecker : public juce::Thread {
public:
    // Pass the callback to be executed on the UI message thread if an update is found
    UpdateChecker(std::function<void(juce::String)> onUpdateAvailable)
        : juce::Thread("UpdateCheckerThread"), callback(onUpdateAvailable) {
        startThread();
    }
    
    ~UpdateChecker() {
        stopThread(1000);
    }
    
    // Helper to extract numeric value from semantic version (e.g. 3.1.0 -> 310)
    int parseVersion(const juce::String& ver) {
        juce::String cleanVer = ver.retainCharacters("0123456789.");
        auto parts = juce::StringArray::fromTokens(cleanVer, ".", "");
        int score = 0;
        if (parts.size() > 0) score += parts[0].getIntValue() * 10000;
        if (parts.size() > 1) score += parts[1].getIntValue() * 100;
        if (parts.size() > 2) score += parts[2].getIntValue() * 1;
        return score;
    }

private:
    void run() override {
        // Fetch raw version text file
        juce::URL versionUrl("https://geek.asmrtop.cn/version.txt");
        juce::var result;
        
        auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                       .withConnectionTimeoutMs(3000)
                       .withNumRedirectsToFollow(2);
                       
        std::unique_ptr<juce::InputStream> stream(versionUrl.createInputStream(options));
        if (stream != nullptr) {
            juce::String fetchedVersion = stream->readEntireStreamAsString().trim();
            
            if (fetchedVersion.isNotEmpty() && (fetchedVersion.contains(".") || fetchedVersion.contains(","))) {
                int currentVer = parseVersion(JucePlugin_VersionString);
                int onlineVer = parseVersion(fetchedVersion);
                
                if (onlineVer > currentVer && onlineVer > 0) {
                    // Tell the GUI safely
                    juce::MessageManager::callAsync([cb = callback, versionStr = fetchedVersion.retainCharacters("0123456789.")]() {
                        if (cb) cb(versionStr);
                    });
                }
            }
        }
    }
    
    std::function<void(juce::String)> callback;
};
