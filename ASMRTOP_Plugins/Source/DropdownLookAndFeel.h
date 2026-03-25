#pragma once
#include <JuceHeader.h>

// Custom LookAndFeel that forces ComboBox popups to appear as dropdowns
// below the component, fixing positioning issues in VST plugin windows.
class DropdownLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DropdownLookAndFeel()
    {
        // Dark theme colors
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xFF1E2229));
        setColour(juce::PopupMenu::textColourId, juce::Colours::white);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xFFE91E63));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour(juce::PopupMenu::headerTextColourId, juce::Colour(0xFFE91E63));
    }
    
    // Force popup to show below the ComboBox like a proper dropdown
    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu (juce::ComboBox& box, juce::Label&) override
    {
        auto screenBounds = box.getScreenBounds();
        
        return juce::PopupMenu::Options()
            .withTargetScreenArea(screenBounds)
            .withMinimumWidth(box.getWidth())
            .withMaximumNumColumns(1)
            .withPreferredPopupDirection(juce::PopupMenu::Options::PopupDirection::downwards);
    }
};
