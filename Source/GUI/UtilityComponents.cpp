/*
 ==============================================================================
 
 UtilityComponents.cpp
 Created: 14 Jun 2023 12:30:00am
 Author:  Keith Hetrick
 
 ==============================================================================
 */

#include "UtilityComponents.h"

//==============================================================================
Placeholder::Placeholder()
{
    juce::Random r;
    customColor = juce::Colour(r.nextInt(255), r.nextInt(255), r.nextInt(255));
}

void Placeholder::paint(juce::Graphics& g)
{
    g.fillAll(customColor);
}
//==============================================================================
RotarySlider::RotarySlider() :
juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
             juce::Slider::TextEntryBoxPosition::NoTextBox)
{ }
