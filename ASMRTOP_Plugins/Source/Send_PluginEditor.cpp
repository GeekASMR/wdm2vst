#include "Send_PluginProcessor.h"
#include "Send_PluginEditor.h"
#include <thread>

AsmrtopSendAudioProcessorEditor::AsmrtopSendAudioProcessorEditor(AsmrtopSendAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    using L = Asmrtop::Lang;
    setSize(500, 400);

    // Language toggle
    langBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
    langBtn.onClick = [this] { Asmrtop::Lang::toggle(); langBtn.setButtonText(L::t(L::LANG_SWITCH)); repaint(); };
    langBtn.setButtonText(L::t(L::LANG_SWITCH));
    addAndMakeVisible(langBtn);

    // Channel selector
    for (int i = 0; i < 8; ++i)
        channelSelector.addItem("P2P Channel " + juce::String(i + 1), i + 1);
    channelSelector.setSelectedId(audioProcessor.channelId + 1, juce::dontSendNotification);
    channelSelector.onChange = [this] { audioProcessor.channelId = channelSelector.getSelectedId() - 1; };
    addAndMakeVisible(channelSelector);

    // Mode toggles
    ipcToggle.setButtonText(L::t(L::IPC));
    udpToggle.setButtonText(L::t(L::LAN));
    wsToggle.setButtonText(L::t(L::MOBILE));
    ipcToggle.setToggleState(audioProcessor.enableIPC, juce::dontSendNotification);
    udpToggle.setToggleState(audioProcessor.enableUDP, juce::dontSendNotification);
    wsToggle.setToggleState(audioProcessor.enableWS, juce::dontSendNotification);
    ipcToggle.onClick = [this] { audioProcessor.enableIPC = ipcToggle.getToggleState(); };
    udpToggle.onClick = [this] { audioProcessor.enableUDP = udpToggle.getToggleState(); };
    wsToggle.onClick  = [this] { audioProcessor.enableWS  = wsToggle.getToggleState(); };
    addAndMakeVisible(ipcToggle);
    addAndMakeVisible(udpToggle);
    addAndMakeVisible(wsToggle);

    // Network settings
    udpIPEditor.setText(audioProcessor.udpTargetIP, false);
    udpIPEditor.onTextChange = [this] { audioProcessor.udpTargetIP = udpIPEditor.getText(); };
    addAndMakeVisible(udpIPEditor);

    udpPortEditor.setText(juce::String(audioProcessor.udpPort), false);
    udpPortEditor.onTextChange = [this] { audioProcessor.udpPort = udpPortEditor.getText().getIntValue(); };
    addAndMakeVisible(udpPortEditor);

    wsPortEditor.setText(juce::String(audioProcessor.wsPort), false);
    wsPortEditor.onTextChange = [this] { audioProcessor.wsPort = wsPortEditor.getText().getIntValue(); };
    addAndMakeVisible(wsPortEditor);

    // Start/Stop button
    startStopBtn.onClick = [this] {
        if (audioProcessor.isSending()) audioProcessor.stopSending();
        else audioProcessor.startSending();
        updateStartStopButton();
    };
    addAndMakeVisible(startStopBtn);
    updateStartStopButton();

    // QR button
    qrBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF26C6DA));
    qrBtn.onClick = [this] { showQRCode(); };
    addAndMakeVisible(qrBtn);

    // Firewall button
    firewallBtn.onClick = [this] {
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
    portOpen = checkFirewallPort(audioProcessor.wsPort);

    startTimerHz(30);
}

AsmrtopSendAudioProcessorEditor::~AsmrtopSendAudioProcessorEditor() { stopTimer(); }

void AsmrtopSendAudioProcessorEditor::timerCallback()
{
    float newL = audioProcessor.peakL.load(std::memory_order_relaxed);
    float newR = audioProcessor.peakR.load(std::memory_order_relaxed);
    if (newL > displayPeakL) displayPeakL = juce::jmin(1.0f, newL);
    else displayPeakL = juce::jmax(0.0f, displayPeakL - 0.04f);
    if (newR > displayPeakR) displayPeakR = juce::jmin(1.0f, newR);
    else displayPeakR = juce::jmax(0.0f, displayPeakR - 0.04f);

    updateStartStopButton();
    
    // Check port status every ~2 seconds
    static int portCheckCounter = 0;
    if (++portCheckCounter >= 60) {
        portCheckCounter = 0;
        portOpen = checkFirewallPort(audioProcessor.wsPort);
    }
    if (portOpen) {
        if (firewallBtn.getButtonText() != "Fixing...")
        {
            firewallBtn.setButtonText("Done");
            firewallBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF4CAF50));
            firewallBtn.setEnabled(false);
        }
    } else if (firewallBtn.getButtonText() != "Fixing...") {
        firewallBtn.setButtonText("Fix FW");
        firewallBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFFE91E63));
        firewallBtn.setEnabled(true);
    }
    
    repaint();
}

void AsmrtopSendAudioProcessorEditor::updateStartStopButton()
{
    using L = Asmrtop::Lang;
    bool sending = audioProcessor.isSending();
    startStopBtn.setButtonText(sending ? L::t(L::STOP) : L::t(L::START));
    startStopBtn.setColour(juce::TextButton::buttonColourId,
        sending ? juce::Colour(0xFFE91E63) : juce::Colour(0xFF4CAF50));
    // Update toggle texts on repaint
    ipcToggle.setButtonText(L::t(L::IPC));
    udpToggle.setButtonText(L::t(L::LAN));
    wsToggle.setButtonText(L::t(L::MOBILE));
    qrBtn.setButtonText(L::t(L::QR_BTN));
    if (firewallBtn.getButtonText() != L::t(L::FIXING)) {
        if (portOpen) firewallBtn.setButtonText(L::t(L::FW_DONE));
        else firewallBtn.setButtonText(L::t(L::FIX_FW));
    }
}

void AsmrtopSendAudioProcessorEditor::showQRCode()
{
    auto localIP = juce::IPAddress::getLocalAddress().toString();
    int port = audioProcessor.wsPort;
    juce::String url = "http://" + localIP + ":" + juce::String(port);

    // Copy to clipboard
    juce::SystemClipboard::copyTextToClipboard(url);

    auto* qrComp = new QRCodeComponent(url);
    
    juce::DialogWindow::LaunchOptions opts;
    opts.content.setOwned(qrComp);
    opts.dialogTitle = "Mobile Monitor - Scan QR Code";
    opts.dialogBackgroundColour = juce::Colour(0xFF0F1115);
    opts.escapeKeyTriggersCloseButton = true;
    opts.useNativeTitleBar = true;
    opts.resizable = false;
    opts.launchAsync();
}

void AsmrtopSendAudioProcessorEditor::paint(juce::Graphics& g)
{
    using L = Asmrtop::Lang;
    g.fillAll(juce::Colour(0xFF0F1115));

    // Header
    g.setColour(juce::Colour(0xFF191C22));
    g.fillRect(0, 0, getWidth(), 65);
    g.setColour(juce::Colour(0xFF00BCD4));
    g.fillRect(0, 63, getWidth(), 2);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText(L::t(L::SEND_TITLE), 20, 17, getWidth() - 120, 30, juce::Justification::left);

    g.setColour(juce::Colour(0xFF555555));
    g.setFont(12.0f);
    g.drawText(juce::String("v") + JucePlugin_VersionString, getWidth()-95, 25, 40, 15, juce::Justification::right);

    // Labels
    g.setColour(juce::Colour(0xFFAAAAAA));
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText(L::t(L::CHANNEL), 25, 77, 100, 18, juce::Justification::left);
    g.drawText(L::t(L::MODE), 25, 115, 100, 18, juce::Justification::left);
    g.drawText(L::t(L::LAN_TARGET), 25, 155, 100, 18, juce::Justification::left);
    g.drawText(L::t(L::UDP_PORT), 320, 155, 100, 18, juce::Justification::left);
    g.drawText(L::t(L::WS_PORT), 25, 195, 100, 18, juce::Justification::left);

    // Status panel
    g.setColour(juce::Colour(0xFF16191F));
    g.fillRoundedRectangle(20, 235, 460, 60, 8.0f);
    g.setColour(juce::Colour(0x30FFFFFF));
    g.drawRoundedRectangle(20, 235, 460, 60, 8.0f, 1.0f);

    g.setColour(juce::Colour(0xFF00BCD4));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(L::t(L::STATUS), 35, 240, 80, 16, juce::Justification::left);

    auto& s = audioProcessor.getSender();
    juce::String status;
    if (!s.isActive()) status = L::t(L::IDLE);
    else {
        juce::StringArray parts;
        if (audioProcessor.enableIPC) parts.add(s.isIPCConnected() ? "IPC OK" : "IPC...");
        if (s.isUDPActive()) {
            parts.add("UDP > " + audioProcessor.udpTargetIP + ":" + juce::String(audioProcessor.udpPort));
        }
        if (s.isWSRunning()) {
            auto localIP = juce::IPAddress::getLocalAddress().toString();
            parts.add("WS http://" + localIP + ":" + juce::String(s.getWSPort()) +
                      " (" + juce::String(s.getWSClientCount()) + " " + L::t(L::CLIENTS) + ")");
        }
        status = parts.joinIntoString("  |  ");
        if (status.isEmpty()) status = L::t(L::ACTIVE);
    }
    g.setColour(juce::Colours::white);
    g.setFont(13.0f);
    g.drawText(status, 35, 258, 430, 22, juce::Justification::left, true);

    // Meters
    g.setColour(juce::Colour(0xFF0A0C0F));
    g.fillRoundedRectangle(25, 315, 450, 8, 4.0f);
    g.fillRoundedRectangle(25, 329, 450, 8, 4.0f);

    float wL = 450.0f * displayPeakL;
    float wR = 450.0f * displayPeakR;
    g.setColour(juce::Colour(0xFF00BCD4));
    if (wL > 0) g.fillRoundedRectangle(25, 315, wL, 8, 4.0f);
    if (wR > 0) g.fillRoundedRectangle(25, 329, wR, 8, 4.0f);

    g.setColour(juce::Colour(0xFF555555));
    g.setFont(11.0f);
    g.drawText("L", 15, 313, 10, 14, juce::Justification::left);
    g.drawText("R", 15, 327, 10, 14, juce::Justification::left);

    // Hints area
    int hintY = 350;
    g.setFont(11.0f);

    if (audioProcessor.enableUDP) {
        g.setColour(juce::Colour(0xFF555555));
        g.drawText(L::t(L::LAN_HINT), 25, hintY, 450, 16, juce::Justification::left);
        hintY += 16;
    }

    if (audioProcessor.enableWS && s.isWSRunning()) {
        g.setColour(juce::Colour(0xFF26C6DA));
        g.setFont(11.0f);
        g.drawText(L::t(L::MOBILE_HINT), 25, hintY, 360, 14, juce::Justification::left);
    }
}

void AsmrtopSendAudioProcessorEditor::resized()
{
    langBtn.setBounds(getWidth() - 50, 20, 35, 24);

    channelSelector.setBounds(100, 73, 220, 28);
    startStopBtn.setBounds(335, 73, 70, 28);
    qrBtn.setBounds(415, 73, 65, 28);

    ipcToggle.setBounds(100, 112, 70, 24);
    udpToggle.setBounds(180, 112, 70, 24);
    wsToggle.setBounds(260, 112, 90, 24);

    udpIPEditor.setBounds(125, 152, 180, 24);
    udpPortEditor.setBounds(395, 152, 85, 24);
    wsPortEditor.setBounds(100, 192, 85, 24);
    firewallBtn.setBounds(200, 192, 90, 24);
}

bool AsmrtopSendAudioProcessorEditor::checkFirewallPort(int port)
{
    // Simple check: if WS server is running, we assume port is OK
    // (actual verification would need admin rights)
    if (audioProcessor.getSender().isWSRunning()) return true;
    return false;
}

void AsmrtopSendAudioProcessorEditor::openFirewallPort(int port)
{
#ifdef JUCE_WINDOWS
    // PowerShell script to:
    // 1. Remove ALL Block rules for Studio One (any version)
    // 2. Add Allow rules for port range and Studio One exe
    juce::String ps = 
        "Get-NetFirewallRule -Direction Inbound -ErrorAction SilentlyContinue | "
        "Where-Object { $_.DisplayName -like '*Studio One*' -and $_.Action -eq 'Block' } | "
        "Remove-NetFirewallRule -ErrorAction SilentlyContinue; "
        "netsh advfirewall firewall add rule name='ASMRTOP Monitor TCP' dir=in action=allow protocol=tcp localport=" + juce::String(port) + " profile=any; "
        "netsh advfirewall firewall add rule name='ASMRTOP Monitor UDP' dir=in action=allow protocol=udp localport=" + juce::String(port - 10) + "-" + juce::String(port + 10) + " profile=any; "
        "Get-ChildItem 'C:\\','D:\\' -Recurse -Filter 'Studio One.exe' -Depth 5 -ErrorAction SilentlyContinue | "
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
