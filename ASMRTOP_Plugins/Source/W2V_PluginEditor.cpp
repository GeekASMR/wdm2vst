#include "W2V_PluginProcessor.h"
#include "W2V_PluginEditor.h"
#include "PluginBranding.h"

AsmrtopWdm2VstAudioProcessorEditor::AsmrtopWdm2VstAudioProcessorEditor (AsmrtopWdm2VstAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    using L = Asmrtop::Lang;

    // Language toggle
    langBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
    langBtn.onClick = [this] { Asmrtop::Lang::toggle(); langBtn.setButtonText(Asmrtop::Lang::t(Asmrtop::Lang::LANG_SWITCH)); refreshDeviceList(); repaint(); };
    langBtn.setButtonText(L::t(L::LANG_SWITCH));
    addAndMakeVisible(langBtn);

    addAndMakeVisible (deviceComboBox);
    deviceComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF1E2229));
    deviceComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    deviceComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    deviceComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xFFE91E63));
    deviceComboBox.setLookAndFeel(&dropdownLnf);

    addAndMakeVisible (volumeSlider);
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    volumeSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFFE91E63));
    volumeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    volumeSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    volumeSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.vts, "gain", volumeSlider);

    addAndMakeVisible (limiterToggle);
    limiterToggle.setButtonText("Limiter");
    limiterToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    limiterToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFFE91E63));
    limiterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.vts, "limiter", limiterToggle);

    addAndMakeVisible (latencyModeComboBox);
    latencyModeComboBox.addItem("Safe (8ms+)", 1);
    latencyModeComboBox.addItem("Fast (4ms+)", 2);
    latencyModeComboBox.addItem("Extreme (2ms+)", 3);
    latencyModeComboBox.addItem("Insane (1ms+ PRIO)", 4);
    latencyModeComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF1E2229));
    latencyModeComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    latencyModeComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    latencyModeComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xFFE91E63));
    latencyModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.vts, "latencyMode", latencyModeComboBox);

    // Populate combobox with input devices
    refreshDeviceList();
    deviceComboBox.addListener(this);

    setSize (500, 300);
    startTimerHz(30);
}

void AsmrtopWdm2VstAudioProcessorEditor::refreshDeviceList()
{
    using L = Asmrtop::Lang;
    
    // Save current selection
    juce::String currentText = deviceComboBox.getText();
    int currentId = deviceComboBox.getSelectedId();
    
    deviceComboBox.clear(juce::dontSendNotification);
    inputDeviceNames.clear();
    deviceComboBox.addItem("- None -", 1);
    
    auto currentTypeName = audioProcessor.deviceManager.getCurrentAudioDeviceType();
    for (auto* t : audioProcessor.deviceManager.getAvailableDeviceTypes())
    {
        if (t != nullptr && t->getTypeName() == currentTypeName)
        {
            t->scanForDevices();
            juce::StringArray rawNames = t->getDeviceNames(true);
            
            // Helper: sort names, putting our "Virtual" devices first
            auto sortDevices = [](juce::StringArray& names) {
                juce::StringArray ours, others;
                for (auto& n : names) {
                    if (n.containsIgnoreCase("Virtual") && n.containsIgnoreCase("Audio Router"))
                        ours.add(n);
                    else
                        others.add(n);
                }
                ours.sortNatural();
                others.sortNatural();
                names.clear();
                names.addArray(ours);
                names.addArray(others);
            };
            
            // --- 1. IPC CHANNELS FIRST ---
            {
                bool anyFound = false;
                for (int i = 0; i < 4; ++i) {
                    juce::String name = Asmrtop::SharedMemoryBridge::getIpcChannelName("PLAY", i);
                    if (name.isNotEmpty()) {
                        if (!anyFound) {
                            deviceComboBox.addSeparator();
                            deviceComboBox.addSectionHeading(L::t(L::SEC_IPC));
                            anyFound = true;
                        }
                        deviceComboBox.addItem(name + " [IPC]", 9000 + i);
                    }
                }
            }
            
            // Classify raw device names into categories
            juce::StringArray loopbackDevices, recordingDevices;
            for (auto& n : rawNames) {
                if (n.containsIgnoreCase("Loopback") || n.containsIgnoreCase("Speakers") || 
                    n.containsIgnoreCase("\xe6\x89\xac\xe5\xa3\xb0\xe5\x99\xa8") || n.containsIgnoreCase("OUT ")) {
                    loopbackDevices.add(n);
                } else {
                    recordingDevices.add(n);
                }
            }
            sortDevices(loopbackDevices);
            sortDevices(recordingDevices);
            
            int itemId = 2;
            
            // --- 3. LOOPBACK ---
            if (loopbackDevices.size() > 0) {
                deviceComboBox.addSeparator();
                deviceComboBox.addSectionHeading(L::t(L::SEC_LOOPBACK));
                for (auto& n : loopbackDevices) {
                    inputDeviceNames.add(n);
                    deviceComboBox.addItem(n, itemId++);
                }
            }
            
            // --- 4. STANDARD RECORDING ---
            if (recordingDevices.size() > 0) {
                deviceComboBox.addSeparator();
                deviceComboBox.addSectionHeading(L::t(L::SEC_RECORDING));
                for (auto& n : recordingDevices) {
                    inputDeviceNames.add(n);
                    deviceComboBox.addItem(n, itemId++);
                }
            }
            
            // Restore selection
            if (audioProcessor.isIPCMode())
            {
                deviceComboBox.setText(audioProcessor.getCurrentDeviceName(), juce::dontSendNotification);
            }
            else if (currentId > 0)
            {
                // Try to find the same device by name
                int idx = inputDeviceNames.indexOf(currentText);
                if (idx >= 0)
                    deviceComboBox.setSelectedId(idx + 2, juce::dontSendNotification);
                else
                    deviceComboBox.setSelectedId(1, juce::dontSendNotification);
            }
            else
            {
                auto* currentDevice = audioProcessor.deviceManager.getCurrentAudioDevice();
                if (currentDevice != nullptr && currentDevice->getName().isNotEmpty()) {
                    int index = inputDeviceNames.indexOf(currentDevice->getName());
                    deviceComboBox.setSelectedId(index >= 0 ? index + 2 : 1, juce::dontSendNotification);
                } else {
                    deviceComboBox.setSelectedId(1, juce::dontSendNotification);
                }
            }
            break;
        }
    }
}

AsmrtopWdm2VstAudioProcessorEditor::~AsmrtopWdm2VstAudioProcessorEditor()
{
    stopTimer();
    deviceComboBox.removeListener(this);
    deviceComboBox.setLookAndFeel(nullptr);
}

void AsmrtopWdm2VstAudioProcessorEditor::timerCallback()
{
    float newPeakL = audioProcessor.currentPeakLevelLeft.exchange(0.0f);
    float newPeakR = audioProcessor.currentPeakLevelRight.exchange(0.0f);
    
    if (newPeakL > displayedPeakL) displayedPeakL = juce::jmin(1.0f, newPeakL);
    else displayedPeakL = juce::jmax(0.0f, displayedPeakL - 0.05f);
    
    if (newPeakR > displayedPeakR) displayedPeakR = juce::jmin(1.0f, newPeakR);
    else displayedPeakR = juce::jmax(0.0f, displayedPeakR - 0.05f);
    
    // Grey out latency mode when not in IPC
    bool isDirectMode = audioProcessor.isIPCMode();
    latencyModeComboBox.setEnabled(isDirectMode);
    latencyModeComboBox.setAlpha(isDirectMode ? 1.0f : 0.4f);
        
    repaint(20, 150, 460, 140);
}

void AsmrtopWdm2VstAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &deviceComboBox)
    {
        int selectedId = deviceComboBox.getSelectedId();
        juce::AudioDeviceManager::AudioDeviceSetup setup = audioProcessor.deviceManager.getAudioDeviceSetup();
        juce::String selectedName = deviceComboBox.getText();
        
        if (selectedName.contains("[IPC]"))
        {
            int channelId = selectedId - 9000;
            audioProcessor.enableIPCMode(channelId, selectedName);
            audioProcessor.deviceManager.closeAudioDevice();
        }
        else
        {
            audioProcessor.disableIPCMode();
            if (selectedId == 1 || selectedId == 0) {
                audioProcessor.deviceManager.closeAudioDevice();
            } else {
                setup.inputDeviceName = selectedName;
                audioProcessor.deviceManager.setAudioDeviceSetup(setup, true);
            }
        }
    }
}

void AsmrtopWdm2VstAudioProcessorEditor::paint (juce::Graphics& g)
{
    using L = Asmrtop::Lang;
    // Background - Dark Mode
    g.fillAll (juce::Colour(0xFF0F1115)); 

    // Header Panel
    g.setColour (juce::Colour(0xFF191C22));
    g.fillRect (0, 0, getWidth(), 65);
    g.setColour (juce::Colour(0xFFE91E63)); // Neon Pink Accent
    g.fillRect (0, 63, getWidth(), 2);

    // Title text
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font(24.0f, juce::Font::bold));
#if IS_INSTRUMENT_MODE
    g.drawText (L::t(L::W2V_TITLE) + " (INST)", 20, 17, getWidth()-120, 30, juce::Justification::left);
#else
    g.drawText (L::t(L::W2V_TITLE), 20, 17, getWidth()-120, 30, juce::Justification::left);
#endif
    
    // Version (left of lang button)
    g.setColour (juce::Colour(0xFF555555));
    g.setFont(12.0f);
    g.drawText(juce::String("v") + JucePlugin_VersionString, getWidth()-95, 25, 40, 15, juce::Justification::right);

    // Labels
    g.setColour (juce::Colour(0xFFAAAAAA));
    g.setFont (juce::Font(14.0f, juce::Font::bold));
    g.drawText(L::t(L::W2V_DEVICE), 25, 85, 100, 20, juce::Justification::left);
    g.drawText(L::t(L::VOLUME), 25, 125, 100, 20, juce::Justification::left);

    // ---------------- LCD Screen bounds -----------------
    g.setColour (juce::Colour(0xFF16191F));
    g.fillRoundedRectangle(20, 155, 460, 135, 8.0f);
    g.setColour (juce::Colour(0x30FFFFFF));
    g.drawRoundedRectangle(20, 155, 460, 135, 8.0f, 1.0f);

    juce::String diagStr = audioProcessor.getDeviceDiagnostics();
    juce::StringArray tokens;
    tokens.addTokens(diagStr, "|", "");
    juce::String devName = tokens.size() > 0 ? tokens[0].trim() : "None";
    juce::String stats = "";
    for(int i=1; i<tokens.size(); ++i) stats += tokens[i].trim() + "   ";

    g.setColour (juce::Colour(0xFFE91E63)); 
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(L::t(L::W2V_DEVICE), 35, 175, 200, 15, juce::Justification::left);

    g.setColour (juce::Colours::white);
    g.setFont(16.0f);
    g.drawText(devName, 35, 195, 300, 22, juce::Justification::left, true);

    g.setColour (juce::Colour(0xFFAAAAAA));
    g.setFont(13.0f);
    g.drawText(stats, 35, 220, 300, 16, juce::Justification::left, true);

    // Latency section
    g.setColour (juce::Colour(0xFF555555));
    g.setFont(11.0f);
    g.drawText("HOST BLOCK", 350, 175, 110, 16, juce::Justification::right);
    g.setColour (juce::Colours::white);
    g.setFont(13.0f);
    g.drawText(juce::String(audioProcessor.getLastNumSamples()), 350, 187, 110, 16, juce::Justification::right);

    g.setColour (juce::Colour(0xFF555555));
    g.setFont(11.0f);
    g.drawText("CACHED BUF", 350, 203, 110, 16, juce::Justification::right);
    g.setColour (juce::Colours::white);
    g.setFont(13.0f);
    g.drawText(juce::String(audioProcessor.getAvailableSamples()), 350, 215, 110, 16, juce::Justification::right);

    g.setColour (juce::Colour(0xFF555555));
    g.setFont(11.0f);
    g.drawText("SYNC LATENCY", 350, 230, 110, 16, juce::Justification::right);
    g.setColour (juce::Colour(0xFFE91E63));
    g.setFont(16.0f);
    g.drawText(juce::String(audioProcessor.getLatencyMs(), 1) + " ms", 350, 240, 110, 22, juce::Justification::right);

    // Meters
    g.setColour (juce::Colour(0xFF0A0C0F));
    g.fillRoundedRectangle(35, 264, 430, 6, 3.0f);
    g.fillRoundedRectangle(35, 276, 430, 6, 3.0f);

    float wL = 430.0f * displayedPeakL;
    float wR = 430.0f * displayedPeakR;
    g.setColour (juce::Colour(0xFF4CAF50).withMultipliedSaturation(1.5f));
    if (wL > 0) g.fillRoundedRectangle(35, 264, wL, 6, 3.0f);
    if (wR > 0) g.fillRoundedRectangle(35, 276, wR, 6, 3.0f);
}

void AsmrtopWdm2VstAudioProcessorEditor::resized()
{
    langBtn.setBounds(getWidth() - 50, 20, 35, 24);
    deviceComboBox.setBounds (90, 80, 390, 30);
    volumeSlider.setBounds (85, 120, 190, 30);
    latencyModeComboBox.setBounds (280, 123, 110, 24);
    limiterToggle.setBounds (400, 125, 80, 20);
}
