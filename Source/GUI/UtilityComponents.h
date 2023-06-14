/*
  ==============================================================================

    UtilityComponents.h
    Created: 14 Jun 2023 12:30:00am
    Author:  Keith Hetrick

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
struct Placeholder : juce::Component
{
    Placeholder();
    
    void paint(juce::Graphics& g) override;

    juce::Colour customColor;
};

struct RotarySlider : juce::Slider
{
    RotarySlider();
};
