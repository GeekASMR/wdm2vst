#pragma once
#include <JuceHeader.h>
#include "QRCode.h"
#include "Localization.h"

// QR Code popup window - shared by Send and VST2WDM plugins
class QRCodeComponent : public juce::Component
{
public:
    QRCodeComponent(const juce::String& url) : connectURL(url)
    {
        auto qr = QR::generate(url.toStdString());
        qrSize = qr.size;
        qrModules = qr.modules;
        setSize(360, 440);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xFF0F1115));

        using L = Asmrtop::Lang;
        g.setColour(juce::Colour(0xFF00BCD4));
        g.setFont(juce::Font(16.0f, juce::Font::bold));
        g.drawText(L::t(L::QR_TITLE), 0, 15, getWidth(), 24, juce::Justification::centred);

        // QR code
        if (qrSize > 0 && (int)qrModules.size() == qrSize * qrSize) {
            int cellSize = 280 / qrSize;
            int qrPixels = cellSize * qrSize;
            int offsetX = (getWidth() - qrPixels) / 2;
            int offsetY = 50;

            // White background with margin
            g.setColour(juce::Colours::white);
            g.fillRect(offsetX - 8, offsetY - 8, qrPixels + 16, qrPixels + 16);

            // Draw modules
            g.setColour(juce::Colours::black);
            for (int y = 0; y < qrSize; y++)
                for (int x = 0; x < qrSize; x++)
                    if (qrModules[y * qrSize + x])
                        g.fillRect(offsetX + x * cellSize, offsetY + y * cellSize, cellSize, cellSize);
        }

        // URL text
        g.setColour(juce::Colour(0xFFAAAAAA));
        g.setFont(14.0f);
        g.drawText(connectURL, 0, 360, getWidth(), 20, juce::Justification::centred);

        // Hint
        g.setColour(juce::Colour(0xFF555555));
        g.setFont(11.0f);
        g.drawText(L::t(L::QR_HINT1), 0, 385, getWidth(), 16, juce::Justification::centred);
        g.drawText(L::t(L::QR_HINT2), 0, 405, getWidth(), 16, juce::Justification::centred);
    }

private:
    juce::String connectURL;
    int qrSize = 0;
    std::vector<bool> qrModules;
};
