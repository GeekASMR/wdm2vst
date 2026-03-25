#include "Recv_PluginProcessor.h"
#include "Recv_PluginEditor.h"

AsmrtopRecvAudioProcessorEditor::AsmrtopRecvAudioProcessorEditor(AsmrtopRecvAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    using L = Asmrtop::Lang;
    setSize(500, 320);

    // Language toggle
    langBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF333333));
    langBtn.onClick = [this] { Asmrtop::Lang::toggle(); langBtn.setButtonText(Asmrtop::Lang::t(Asmrtop::Lang::LANG_SWITCH)); repaint(); };
    langBtn.setButtonText(L::t(L::LANG_SWITCH));
    addAndMakeVisible(langBtn);

    for (int i = 0; i < 8; ++i)
        channelSelector.addItem("P2P Channel " + juce::String(i + 1), i + 1);
    channelSelector.setSelectedId(audioProcessor.channelId + 1, juce::dontSendNotification);
    channelSelector.onChange = [this] { audioProcessor.channelId = channelSelector.getSelectedId() - 1; };
    addAndMakeVisible(channelSelector);

    ipcToggle.setButtonText(L::t(L::IPC));
    udpToggle.setButtonText(L::t(L::LAN));
    ipcToggle.setToggleState(audioProcessor.enableIPC, juce::dontSendNotification);
    udpToggle.setToggleState(audioProcessor.enableUDP, juce::dontSendNotification);
    ipcToggle.onClick = [this] { audioProcessor.enableIPC = ipcToggle.getToggleState(); };
    udpToggle.onClick = [this] { audioProcessor.enableUDP = udpToggle.getToggleState(); };
    addAndMakeVisible(ipcToggle);
    addAndMakeVisible(udpToggle);

    udpPortEditor.setText(juce::String(audioProcessor.udpPort), false);
    udpPortEditor.onTextChange = [this] { audioProcessor.udpPort = udpPortEditor.getText().getIntValue(); };
    addAndMakeVisible(udpPortEditor);

    startStopBtn.onClick = [this] {
        if (audioProcessor.isReceiving()) audioProcessor.stopReceiving();
        else audioProcessor.startReceiving();
        updateStartStopButton();
    };
    addAndMakeVisible(startStopBtn);
    updateStartStopButton();

    startTimerHz(30);
}

AsmrtopRecvAudioProcessorEditor::~AsmrtopRecvAudioProcessorEditor() { stopTimer(); }

void AsmrtopRecvAudioProcessorEditor::timerCallback()
{
    float newL = audioProcessor.peakL.load(std::memory_order_relaxed);
    float newR = audioProcessor.peakR.load(std::memory_order_relaxed);
    if (newL > displayPeakL) displayPeakL = juce::jmin(1.0f, newL);
    else displayPeakL = juce::jmax(0.0f, displayPeakL - 0.04f);
    if (newR > displayPeakR) displayPeakR = juce::jmin(1.0f, newR);
    else displayPeakR = juce::jmax(0.0f, displayPeakR - 0.04f);

    updateStartStopButton();
    repaint();
}

void AsmrtopRecvAudioProcessorEditor::updateStartStopButton()
{
    using L = Asmrtop::Lang;
    bool recv = audioProcessor.isReceiving();
    startStopBtn.setButtonText(recv ? L::t(L::STOP) : L::t(L::START));
    startStopBtn.setColour(juce::TextButton::buttonColourId,
        recv ? juce::Colour(0xFFE91E63) : juce::Colour(0xFF4CAF50));
    ipcToggle.setButtonText(L::t(L::IPC));
    udpToggle.setButtonText(L::t(L::LAN));
}

void AsmrtopRecvAudioProcessorEditor::paint(juce::Graphics& g)
{
    using L = Asmrtop::Lang;
    g.fillAll(juce::Colour(0xFF0F1115));

    // Header
    g.setColour(juce::Colour(0xFF191C22));
    g.fillRect(0, 0, getWidth(), 65);
    g.setColour(juce::Colour(0xFFFF9800)); // Orange accent for Receive
    g.fillRect(0, 63, getWidth(), 2);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText(L::t(L::RECV_TITLE), 20, 17, getWidth() - 120, 30, juce::Justification::left);

    g.setColour(juce::Colour(0xFF555555));
    g.setFont(12.0f);
    g.drawText(juce::String("v") + JucePlugin_VersionString, getWidth()-95, 25, 40, 15, juce::Justification::right);

    // Labels
    g.setColour(juce::Colour(0xFFAAAAAA));
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText(L::t(L::CHANNEL), 25, 77, 100, 18, juce::Justification::left);
    g.drawText(L::t(L::MODE), 25, 115, 100, 18, juce::Justification::left);
    g.drawText(L::t(L::UDP_PORT), 25, 155, 100, 18, juce::Justification::left);

    // Status panel
    g.setColour(juce::Colour(0xFF16191F));
    g.fillRoundedRectangle(20, 195, 460, 55, 8.0f);
    g.setColour(juce::Colour(0x30FFFFFF));
    g.drawRoundedRectangle(20, 195, 460, 55, 8.0f, 1.0f);

    g.setColour(juce::Colour(0xFFFF9800));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText(L::t(L::STATUS), 35, 200, 80, 16, juce::Justification::left);

    auto& r = audioProcessor.getReceiver();
    juce::String status;
    if (!r.isActive()) status = L::t(L::IDLE);
    else {
        juce::StringArray parts;
        if (r.isIPCConnected()) parts.add("IPC OK");
        else if (audioProcessor.enableIPC) parts.add("IPC...");
        if (r.isUDPActive()) parts.add("UDP :" + juce::String(audioProcessor.udpPort));
        int avail = r.getAvailableSamples();
        parts.add(L::t(L::BUFFER) + ":" + juce::String(avail));
        status = parts.joinIntoString(" | ");
    }
    g.setColour(juce::Colours::white);
    g.setFont(13.0f);
    g.drawText(status, 35, 217, 430, 20, juce::Justification::left, true);

    // Meters
    g.setColour(juce::Colour(0xFF0A0C0F));
    g.fillRoundedRectangle(25, 265, 450, 8, 4.0f);
    g.fillRoundedRectangle(25, 279, 450, 8, 4.0f);

    float wL = 450.0f * displayPeakL;
    float wR = 450.0f * displayPeakR;
    g.setColour(juce::Colour(0xFFFF9800));
    if (wL > 0) g.fillRoundedRectangle(25, 265, wL, 8, 4.0f);
    if (wR > 0) g.fillRoundedRectangle(25, 279, wR, 8, 4.0f);

    g.setColour(juce::Colour(0xFF555555));
    g.setFont(10.0f);
    g.drawText("L", 15, 263, 10, 12, juce::Justification::left);
    g.drawText("R", 15, 277, 10, 12, juce::Justification::left);
}

void AsmrtopRecvAudioProcessorEditor::resized()
{
    langBtn.setBounds(getWidth() - 50, 20, 35, 24);
    channelSelector.setBounds(100, 73, 280, 28);
    startStopBtn.setBounds(395, 73, 85, 28);
    ipcToggle.setBounds(100, 112, 70, 24);
    udpToggle.setBounds(180, 112, 70, 24);
    udpPortEditor.setBounds(100, 152, 85, 24);
}
