/*
 ==============================================================================
 
 CustomButtons.h
 Created: 14 Jun 2023 12:21:30am
 Author:  Keith Hetrick
 
 ==============================================================================
 */

#pragma once

#include <JuceHeader.h>

//==============================================================================
struct PowerButton : juce::ToggleButton { };

struct AnalyzerButton : juce::ToggleButton
{
    void resized() override;
    
    juce::Path randomPath;
};
