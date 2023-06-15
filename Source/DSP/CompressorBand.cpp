/*
 ==============================================================================
 
 CompressorBand.cpp
 Created: 14 Jun 2023 1:06:11am
 Author:  Keith Hetrick
 
 ==============================================================================
 */

#include "CompressorBand.h"

void CompressorBand::prepare(const juce::dsp::ProcessSpec& spec)
{
    compressor.prepare(spec);
}

void CompressorBand::updateCompressorSettings()
{
    compressor.setAttack    (attack->get());
    compressor.setRelease   (release->get());
    compressor.setThreshold (threshold->get());
    compressor.setRatio     ( ratio->getCurrentChoiceName().getFloatValue() );
}

void CompressorBand::process(juce::AudioBuffer<float>& buffer)
{
    auto block =   juce::dsp::AudioBlock<float>(buffer);
    auto context = juce::dsp::ProcessContextReplacing<float>(block);
    
    context.isBypassed = bypassed->get();
    
    compressor.process(context);
}
