#include "V2W_PluginProcessor.h"
#include "V2W_PluginEditor.h"
#include "PluginBranding.h"
#include <thread>

AsmrtopVst2WdmAudioProcessorEditor::AsmrtopVst2WdmAudioProcessorEditor (AsmrtopVst2WdmAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    using L = Asmrtop::Lang;

    // Language toggle
    langBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
    langBtn.onClick = [this] { Asmrtop::Lang::toggle(); langBtn.setButtonText(Asmrtop::Lang::t(Asmrtop::Lang::LANG_SWITCH)); refreshDeviceList(); repaint(); };
    langBtn.setButtonText(L::t(L::LANG_SWITCH));
    addAndMakeVisible(langBtn);

    addAndMakeVisible(updateBtn);
    updateBtn.setButtonText("NEW VST!");
    updateBtn.setColour(juce::HyperlinkButton::textColourId, juce::Colour(0xFF4CAF50));
    updateBtn.setURL(juce::URL("https://github.com/GeekASMR/wdm2vst/releases/latest"));
    updateBtn.setVisible(false);

    updateChecker = std::make_unique<UpdateChecker>([this](juce::String newVer) {
        updateBtn.setButtonText(juce::String(JucePlugin_VersionString) + " -> " + newVer);
        updateBtn.setVisible(true);
        hasUpdate = true;
        repaint();
    });

    addAndMakeVisible (deviceComboBox);
    deviceComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF1E2229));
    deviceComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    deviceComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    deviceComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xFF00E5FF));
    deviceComboBox.setLookAndFeel(&dropdownLnf);

    addAndMakeVisible (volumeSlider);
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 50, 20);
    volumeSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xFF00E5FF));
    volumeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    volumeSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    volumeSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.vts, "gain", volumeSlider);

    addAndMakeVisible (limiterToggle);
    limiterToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    limiterToggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xFF00E5FF));
    limiterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.vts, "limiter", limiterToggle);

    // Mobile Monitor
    mobileToggle.setToggleState(audioProcessor.enableMobile, juce::dontSendNotification);
    mobileToggle.onClick = [this] {
        audioProcessor.enableMobile = mobileToggle.getToggleState();
        if (audioProcessor.enableMobile) audioProcessor.enableMobileMonitor();
        else audioProcessor.disableMobileMonitor();
    };
    addAndMakeVisible(mobileToggle);

    wsPortEditor.setText(juce::String(audioProcessor.wsPort), false);
    wsPortEditor.onTextChange = [this] { audioProcessor.wsPort = wsPortEditor.getText().getIntValue(); };
    addAndMakeVisible(wsPortEditor);

    qrBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF26C6DA));
    qrBtn.onClick = [this] { showQRCode(); };
    addAndMakeVisible(qrBtn);

    firewallBtn.onClick = [this] {
        using L = Asmrtop::Lang;
        firewallBtn.setButtonText(L::t(L::FIXING));
        firewallBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFFFF9800));
        firewallBtn.setEnabled(false);
        std::thread([this] {
            openFirewallPort(audioProcessor.wsPort);
            juce::MessageManager::callAsync([this] {
                using L = Asmrtop::Lang;
                if (portOpen) {
                    firewallBtn.setButtonText(L::t(L::FW_DONE));
                    firewallBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF4CAF50));
                } else {
                    firewallBtn.setButtonText(L::t(L::FW_FAILED));
                    firewallBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFFE91E63));
                    firewallBtn.setEnabled(true);
                }
            });
        }).detach();
    };
    addAndMakeVisible(firewallBtn);

    // Populate combobox with output devices
    refreshDeviceList();
    deviceComboBox.addListener(this);

    setSize (500, 360);
    startTimerHz(30);
}

void AsmrtopVst2WdmAudioProcessorEditor::refreshDeviceList()
{
    using L = Asmrtop::Lang;
    
    // Save current selection
    juce::String currentText = deviceComboBox.getText();
    int currentId = deviceComboBox.getSelectedId();
    
    deviceComboBox.clear(juce::dontSendNotification);
    outputDeviceNames.clear();
    allInputNames.clear();
    deviceComboBox.addItem("- None -", 1);
    
    auto currentTypeName = audioProcessor.deviceManager.getCurrentAudioDeviceType();
    for (auto* t : audioProcessor.deviceManager.getAvailableDeviceTypes())
    {
        if (t != nullptr && t->getTypeName() == currentTypeName)
        {
            t->scanForDevices();
            juce::StringArray rawOuts = t->getDeviceNames(false);
            juce::StringArray rawIns = t->getDeviceNames(true);
            
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
                    juce::String name = Asmrtop::SharedMemoryBridge::getIpcChannelName("REC", i);
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

            // --- 2. DRIVER BRIDGE ---
            {
                juce::StringArray sortedIns;
                for (auto& n : rawIns) sortedIns.add(n);
                sortDevices(sortedIns);
                
                deviceComboBox.addSeparator();
                deviceComboBox.addSectionHeading(L::t(L::SEC_DRIVER_BRIDGE));
                for (auto& n : sortedIns) {
                    allInputNames.add(n);
                    juce::String displayName = n;
                    if (displayName.containsIgnoreCase("Loopback")) {
                        displayName += L::isChinese() ? juce::String::fromUTF8(" [\xe9\xab\x98\xe5\xbb\xb6\xe8\xbf\x9f!]") : " [High Latency!]";
                    }
                    deviceComboBox.addItem(displayName, 5000 + allInputNames.size());
                }
            }
            
            // --- 3. STANDARD PLAYBACK ---
            {
                juce::StringArray sortedOuts;
                for (auto& n : rawOuts) sortedOuts.add(n);
                sortDevices(sortedOuts);
                
                int itemId = 2;
                deviceComboBox.addSeparator();
                deviceComboBox.addSectionHeading(L::t(L::SEC_PLAYBACK));
                for (auto& n : sortedOuts) {
                    outputDeviceNames.add(n);
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
                int idx = outputDeviceNames.indexOf(currentText);
                if (idx >= 0)
                    deviceComboBox.setSelectedId(idx + 2, juce::dontSendNotification);
                else
                    deviceComboBox.setSelectedId(1, juce::dontSendNotification);
            }
            else
            {
                auto* currentDevice = audioProcessor.deviceManager.getCurrentAudioDevice();
                if (currentDevice != nullptr && currentDevice->getName().isNotEmpty()) {
                    int index = outputDeviceNames.indexOf(currentDevice->getName());
                    deviceComboBox.setSelectedId(index >= 0 ? index + 2 : 1, juce::dontSendNotification);
                } else {
                    deviceComboBox.setSelectedId(1, juce::dontSendNotification);
                }
            }
            break;
        }
    }
}

AsmrtopVst2WdmAudioProcessorEditor::~AsmrtopVst2WdmAudioProcessorEditor()
{
    stopTimer();
    deviceComboBox.removeListener(this);
    deviceComboBox.setLookAndFeel(nullptr);
}

void AsmrtopVst2WdmAudioProcessorEditor::timerCallback()
{
    using L = Asmrtop::Lang;
    float newPeakL = audioProcessor.currentPeakLevelLeft.exchange(0.0f);
    float newPeakR = audioProcessor.currentPeakLevelRight.exchange(0.0f);
    
    if (newPeakL > displayedPeakL) displayedPeakL = juce::jmin(1.0f, newPeakL);
    else displayedPeakL = juce::jmax(0.0f, displayedPeakL - 0.05f);
    
    if (newPeakR > displayedPeakR) displayedPeakR = juce::jmin(1.0f, newPeakR);
    else displayedPeakR = juce::jmax(0.0f, displayedPeakR - 0.05f);

    // Update localized texts
    limiterToggle.setButtonText(L::t(L::V2W_LIMITER));
    mobileToggle.setButtonText(L::t(L::MOBILE));
    qrBtn.setButtonText(L::t(L::QR_BTN));
    if (firewallBtn.getButtonText() != L::t(L::FIXING)) {
        if (portOpen) firewallBtn.setButtonText(L::t(L::FW_DONE));
        else firewallBtn.setButtonText(L::t(L::FIX_FW));
    }
    
    if (hasUpdate) {
        flashPhase += 0.15f;
        if (flashPhase > juce::MathConstants<float>::twoPi) flashPhase -= juce::MathConstants<float>::twoPi;
        float alpha = 0.5f + 0.5f * std::sin(flashPhase);
        updateBtn.setColour(juce::HyperlinkButton::textColourId, juce::Colour(0xFF00E5FF).interpolatedWith(juce::Colour(0xFF4CAF50), alpha));
        updateBtn.repaint();
    }
        
    repaint();
}

void AsmrtopVst2WdmAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &deviceComboBox)
    {
        int selectedId = deviceComboBox.getSelectedId();
        juce::AudioDeviceManager::AudioDeviceSetup setup = audioProcessor.deviceManager.getAudioDeviceSetup();
        juce::String selectedName = deviceComboBox.getText();
        
        if (selectedName.contains("[IPC]"))
        {
            audioProcessor.disableIPCMode();
            int channelId = selectedId - 9000;
            audioProcessor.enableIPCMode(channelId, selectedName);
            audioProcessor.deviceManager.closeAudioDevice();
        }
        else
        {
            audioProcessor.disableIPCMode();
            if (selectedId == 1 || selectedId == 0)
            {
                audioProcessor.deviceManager.closeAudioDevice();
            }
            else
            {
                if (selectedId >= 5000 && selectedId < 8000) {
                    selectedName = selectedName.replace(u8" [高延迟!]", "");
                    if (selectedName.contains("Mix 01") || selectedName.contains("MIC 01")) selectedName = "Audio 01";
                    else if (selectedName.contains("Mix 02") || selectedName.contains("MIC 02")) selectedName = "Audio 02";
                    else if (selectedName.contains("Mix 03") || selectedName.contains("MIC 03")) selectedName = "Audio 03";
                    else if (selectedName.contains("Mix 04") || selectedName.contains("MIC 04")) selectedName = "Audio 04";
                    else if (selectedName.contains(u8"麦克风")) selectedName = selectedName.replace(u8"麦克风", u8"扬声器");
                    
                    for (auto& n : outputDeviceNames) {
                        if ((selectedName.contains("Audio 01") && n.contains("Audio 01")) ||
                            (selectedName.contains("Audio 02") && n.contains("Audio 02")) ||
                            (selectedName.contains("Audio 03") && n.contains("Audio 03")) ||
                            (selectedName.contains("Audio 04") && n.contains("Audio 04")) ||
                            (selectedName.contains("Speakers 01") && n.contains("Speakers 01")) ||
                            (selectedName.contains("Speakers 02") && n.contains("Speakers 02")) ||
                            (selectedName.contains("Speakers 03") && n.contains("Speakers 03")) ||
                            (selectedName.contains("Speakers 04") && n.contains("Speakers 04")) ||
                            (selectedName.contains(u8"扬声器") && n.contains(u8"扬声器"))) {
                            selectedName = n;
                            break;
                        }
                    }
                }
                setup.outputDeviceName = selectedName;
                audioProcessor.deviceManager.setAudioDeviceSetup(setup, true);
            }
        }
    }
}

void AsmrtopVst2WdmAudioProcessorEditor::paint (juce::Graphics& g)
{
    using L = Asmrtop::Lang;
    g.fillAll (juce::Colour(0xFF0F1115)); 

    // Header Panel
    g.setColour (juce::Colour(0xFF191C22));
    g.fillRect (0, 0, getWidth(), 65);
    g.setColour (juce::Colour(0xFF00E5FF));
    g.fillRect (0, 63, getWidth(), 2);

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font(24.0f, juce::Font::bold));
    g.drawText (L::t(L::V2W_TITLE), 20, 17, getWidth()-120, 30, juce::Justification::left);

    if (!hasUpdate) {
        g.setColour(juce::Colour(0xFF555555));
        g.setFont(12.0f);
        g.drawText(juce::String("v") + JucePlugin_VersionString, getWidth()-95, 25, 40, 15, juce::Justification::right);
    }

    g.setColour (juce::Colour(0xFFAAAAAA));
    g.setFont (juce::Font(14.0f, juce::Font::bold));
    g.drawText(L::t(L::V2W_DEVICE), 25, 85, 100, 20, juce::Justification::left);
    g.drawText(L::t(L::VOLUME), 25, 125, 100, 20, juce::Justification::left);

    // LCD Screen
    g.setColour (juce::Colour(0xFF16191F));
    g.fillRoundedRectangle(20, 155, 460, 110, 8.0f);
    g.setColour (juce::Colour(0x30FFFFFF));
    g.drawRoundedRectangle(20, 155, 460, 110, 8.0f, 1.0f);

    juce::String diagStr = audioProcessor.getDeviceDiagnostics();
    juce::StringArray tokens;
    tokens.addTokens(diagStr, "|", "");
    juce::String devName = tokens.size() > 0 ? tokens[0].trim() : "None";
    juce::String stats = "";
    for(int i=1; i<tokens.size(); ++i) stats += tokens[i].trim() + "   ";

    g.setColour (juce::Colour(0xFF00E5FF)); 
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(L::t(L::V2W_DEVICE), 35, 165, 200, 16, juce::Justification::left);

    g.setColour (juce::Colours::white);
    g.setFont(16.0f);
    g.drawText(devName, 35, 180, 300, 22, juce::Justification::left, true);

    g.setColour (juce::Colour(0xFFAAAAAA));
    g.setFont(13.0f);
    g.drawText(stats, 35, 200, 300, 16, juce::Justification::left, true);

    // Buffer / Latency
    g.setColour (juce::Colour(0xFF555555));
    g.setFont(11.0f);
    g.drawText(L::t(L::BUFFER), 350, 165, 110, 16, juce::Justification::right);
    g.setColour (juce::Colours::white);
    g.setFont(15.0f);
    g.drawText(juce::String(audioProcessor.getAvailableSamples()), 350, 180, 110, 22, juce::Justification::right);

    g.setColour (juce::Colour(0xFF555555));
    g.setFont(11.0f);
    g.drawText(L::t(L::LATENCY), 350, 200, 110, 16, juce::Justification::right);
    g.setColour (juce::Colour(0xFF00E5FF));
    g.setFont(16.0f);
    g.drawText(juce::String(audioProcessor.getLatencyMs(), 1) + " ms", 350, 215, 110, 22, juce::Justification::right);

    // Meters
    g.setColour (juce::Colour(0xFF0A0C0F));
    g.fillRoundedRectangle(35, 240, 430, 6, 3.0f);
    g.fillRoundedRectangle(35, 252, 430, 6, 3.0f);

    float wL = 430.0f * displayedPeakL;
    float wR = 430.0f * displayedPeakR;
    g.setColour (juce::Colour(0xFF4CAF50).withMultipliedSaturation(1.5f));
    if (wL > 0) g.fillRoundedRectangle(35, 240, wL, 6, 3.0f);
    if (wR > 0) g.fillRoundedRectangle(35, 252, wR, 6, 3.0f);

    // Mobile Monitor section
    g.setColour(juce::Colour(0xFFAAAAAA));
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText(L::t(L::V2W_MONITOR), 25, 275, 100, 20, juce::Justification::left);

    if (audioProcessor.isMobileActive()) {
        auto localIP = juce::IPAddress::getLocalAddress().toString();
        g.setColour(juce::Colour(0xFF26C6DA));
        g.setFont(12.0f);
        g.drawText("http://" + localIP + ":" + juce::String(audioProcessor.wsPort)
                   + " (" + juce::String(audioProcessor.mobileSender.getWSClientCount()) + " " + L::t(L::CLIENTS) + ")",
                   25, 305, 350, 18, juce::Justification::left);
        g.setColour(juce::Colour(0xFF555555));
        g.setFont(11.0f);
        g.drawText(L::t(L::MOBILE_HINT), 25, 325, 350, 16, juce::Justification::left);
    }
}

void AsmrtopVst2WdmAudioProcessorEditor::resized()
{
    langBtn.setBounds(getWidth() - 50, 20, 35, 24);
    updateBtn.setBounds(getWidth() - 145, 22, 90, 20);
    deviceComboBox.setBounds (90, 80, 390, 30);
    volumeSlider.setBounds (85, 120, 300, 30);
    limiterToggle.setBounds (400, 125, 80, 20);

    // Mobile Monitor row
    mobileToggle.setBounds(100, 272, 90, 24);
    wsPortEditor.setBounds(200, 272, 60, 24);
    qrBtn.setBounds(270, 272, 55, 24);
    firewallBtn.setBounds(335, 272, 85, 24);
}

void AsmrtopVst2WdmAudioProcessorEditor::showQRCode()
{
    auto localIP = juce::IPAddress::getLocalAddress().toString();
    int port = audioProcessor.wsPort;
    juce::String url = "http://" + localIP + ":" + juce::String(port);

    juce::SystemClipboard::copyTextToClipboard(url);

    // Reuse QRCodeComponent from Send editor header
    auto* qrComp = new QRCodeComponent(url);
    
    juce::DialogWindow::LaunchOptions opts;
    opts.content.setOwned(qrComp);
    opts.dialogTitle = Asmrtop::Lang::t(Asmrtop::Lang::QR_TITLE).toStdString();
    opts.dialogBackgroundColour = juce::Colour(0xFF0F1115);
    opts.escapeKeyTriggersCloseButton = true;
    opts.useNativeTitleBar = true;
    opts.resizable = false;
    opts.launchAsync();
}

void AsmrtopVst2WdmAudioProcessorEditor::openFirewallPort(int port)
{
#ifdef JUCE_WINDOWS
    juce::String ps = 
        "Get-NetFirewallRule -Direction Inbound -ErrorAction SilentlyContinue | "
        "Where-Object { $_.DisplayName -like '*Studio One*' -and $_.Action -eq 'Block' } | "
        "Remove-NetFirewallRule -ErrorAction SilentlyContinue; "
        "netsh advfirewall firewall add rule name='ASMRTOP Monitor TCP' dir=in action=allow protocol=tcp localport=" + juce::String(port) + " profile=any; "
        "netsh advfirewall firewall add rule name='ASMRTOP Monitor UDP' dir=in action=allow protocol=udp localport=" + juce::String(port - 10) + "-" + juce::String(port + 10) + " profile=any; "
        "Get-ChildItem 'C:\\\\','D:\\\\' -Recurse -Filter 'Studio One.exe' -Depth 5 -ErrorAction SilentlyContinue | "
        "ForEach-Object { "
        "netsh advfirewall firewall add rule name=('ASMRTOP SO '+$_.Directory.Name) dir=in action=allow program=$_.FullName protocol=tcp profile=any "
        "}";

    std::string psUtf8 = ps.toStdString();
    std::string args = "-ExecutionPolicy Bypass -Command \"" + psUtf8 + "\"";

    SHELLEXECUTEINFOA sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = "runas";
    sei.lpFile = "powershell.exe";
    sei.lpParameters = args.c_str();
    sei.nShow = SW_HIDE;
    
    if (ShellExecuteExA(&sei)) {
        if (sei.hProcess) {
            WaitForSingleObject(sei.hProcess, 15000);
            CloseHandle(sei.hProcess);
        }
        portOpen = true;
    }
#endif
}
