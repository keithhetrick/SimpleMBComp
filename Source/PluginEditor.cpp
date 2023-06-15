/*
 ==============================================================================
 
 This file contains the basic framework code for a JUCE plugin editor.
 
 ==============================================================================
 */

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/Params.h"
#include "GUI/Utilities.h"

//==============================================================================
SpectrumAnalyzer::SpectrumAnalyzer(SimpleMBCompAudioProcessor& p) :
audioProcessor(p),
leftPathProducer(audioProcessor.leftChannelFifo),
rightPathProducer(audioProcessor.rightChannelFifo)
{
    const auto& params = audioProcessor.getParameters();
    for( auto param : params )
    {
        param->addListener(this);
    }
    
    startTimerHz(60);
}

SpectrumAnalyzer::~SpectrumAnalyzer()
{
    const auto& params = audioProcessor.getParameters();
    for( auto param : params )
    {
        param->removeListener(this);
    }
}

void SpectrumAnalyzer::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);

    drawBackgroundGrid(g);
    
    auto responseArea = getAnalysisArea();
    
    if( shouldShowFFTAnalysis )
    {
        auto leftChannelFFTPath = leftPathProducer.getPath();
        leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
        
        g.setColour(Colour(97u, 18u, 167u)); //purple-
        g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));
        
        auto rightChannelFFTPath = rightPathProducer.getPath();
        rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
        
        g.setColour(Colour(215u, 201u, 134u));
        g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));
    }
    
    Path border;
    
    border.setUsingNonZeroWinding(false);
    
    border.addRoundedRectangle(getRenderArea(), 4);
    border.addRectangle(getLocalBounds());
    
    g.setColour(Colours::black);
    
    g.fillPath(border);
    
    drawTextLabels(g);
    
    g.setColour(Colours::orange);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);
}

std::vector<float> SpectrumAnalyzer::getFrequencies()
{
    return std::vector<float>
    {
        20, /*30, 40,*/ 50, 100,
        200, /*300, 400,*/ 500, 1000,
        2000, /*3000, 4000,*/ 5000, 10000,
        20000
    };
}

std::vector<float> SpectrumAnalyzer::getGains()
{
    return std::vector<float>
    {
        -24, -12, 0, 12, 24
    };
}

std::vector<float> SpectrumAnalyzer::getXs(const std::vector<float> &freqs, float left, float width)
{
    std::vector<float> xs;
    for( auto f : freqs )
    {
        auto normX = juce::mapFromLog10(f, 20.f, 20000.f);
        xs.push_back( left + width * normX );
    }
    
    return xs;
}

void SpectrumAnalyzer::drawBackgroundGrid(juce::Graphics &g)
{
    using namespace juce;
    auto freqs = getFrequencies();
    
    auto renderArea = getAnalysisArea();
    auto left =   renderArea.getX();
    auto right =  renderArea.getRight();
    auto top =    renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width =  renderArea.getWidth();
    
    auto xs = getXs(freqs, left, width);
    
    g.setColour(Colours::dimgrey);
    for( auto x : xs )
    {
        g.drawVerticalLine(x, top, bottom);
    }
    
    auto gain = getGains();
    
    for( auto gDb : gain )
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey );
        g.drawHorizontalLine(y, left, right);
    }
}

void SpectrumAnalyzer::drawTextLabels(juce::Graphics &g)
{
    using namespace juce;
    g.setColour(Colours::lightgrey);
    const int fontHeight = 10;
    g.setFont  (fontHeight);
    
    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    
    auto top =    renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width =  renderArea.getWidth();
    
    auto freqs = getFrequencies();
    auto xs =    getXs(freqs, left, width);
    
    for( int i = 0; i < freqs.size(); ++i )
    {
        auto f = freqs[i];
        auto x = xs[i];

        bool addK = false;
        String str;
        if( f > 999.f )
        {
            addK = true;
            f /= 1000.f;
        }

        str << f;
        if( addK )
            str << "k";
        str << "Hz";
        
        auto textWidth = g.getCurrentFont().getStringWidth(str);

        Rectangle<int> r;

        r.setSize  (textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY     (1);
        
        g.drawFittedText(str, r, juce::Justification::centred, 1);
    }
    
    auto gain = getGains();

    for( auto gDb : gain )
    {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        
        String str;
        if( gDb > 0 )
            str << "+";
        str << gDb;
        
        auto textWidth = g.getCurrentFont().getStringWidth(str);
        
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);
        
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey );
        
        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
        
        str.clear();
        str << (gDb - 24.f);

        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::centredLeft, 1);
    }
}

void SpectrumAnalyzer::resized()
{
    using namespace juce;
}

void SpectrumAnalyzer::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate)
{
    juce::AudioBuffer<float> tempIncomingBuffer;
    while( leftChannelFifo->getNumCompleteBuffersAvailable() > 0 )
    {
        if( leftChannelFifo->getAudioBuffer(tempIncomingBuffer) )
        {
            auto size = tempIncomingBuffer.getNumSamples();

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, 0),
                                              monoBuffer.getReadPointer(0, size),
                                              monoBuffer.getNumSamples() - size);

            juce::FloatVectorOperations::copy(monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                                              tempIncomingBuffer.getReadPointer(0, 0),
                                              size);
            
            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }
    
    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    const auto binWidth = sampleRate / double(fftSize);

    while( leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0 )
    {
        std::vector<float> fftData;
        if( leftChannelFFTDataGenerator.getFFTData( fftData) )
        {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
        }
    }
    
    while( pathProducer.getNumPathsAvailable() > 0 )
    {
        pathProducer.getPath( leftChannelFFTPath );
    }
}

void SpectrumAnalyzer::timerCallback()
{
    if( shouldShowFFTAnalysis )
    {
        auto fftBounds =  getAnalysisArea().toFloat();
        auto sampleRate = audioProcessor.getSampleRate();
        
        leftPathProducer.process (fftBounds, sampleRate);
        rightPathProducer.process(fftBounds, sampleRate);
    }

    if( parametersChanged.compareAndSetBool(false, true) )
    {
        
    }
    
    repaint();
}

juce::Rectangle<int> SpectrumAnalyzer::getRenderArea()
{
    auto bounds = getLocalBounds();
    
    bounds.removeFromTop   (12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft  (20);
    bounds.removeFromRight (20);
    
    return bounds;
}


juce::Rectangle<int> SpectrumAnalyzer::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}

//==============================================================================
void RotarySliderWithLabels::paint(juce::Graphics &g)
{
    using namespace juce;
    
    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng =   degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;
    
    auto range = getRange();
    
    auto sliderBounds = getSliderBounds();
    
    auto bounds = getLocalBounds();
    
    g.setColour(Colours::blueviolet);
    g.drawFittedText(getName(),
                     bounds.removeFromTop(getTextHeight() + 2),
                     Justification::centredBottom,
                     1);
    
    getLookAndFeel().drawRotarySlider(g,
                                      sliderBounds.getX(),
                                      sliderBounds.getY(),
                                      sliderBounds.getWidth(),
                                      sliderBounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAng,
                                      endAng,
                                      *this);
    
    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;
    
    g.setColour(Colour(0u, 172u, 1u));
    g.setFont(getTextHeight());
    
    auto numChoices = labels.size();
    for( int i = 0; i < numChoices; ++i )
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);
        
        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);
        
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);
        
        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize  (g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY     (r.getY() + getTextHeight());
        
        g.drawFittedText(str, r.toNearestInt(),
                         juce::Justification::centred,
                         1);
    }
    
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();
    
    bounds.removeFromTop(getTextHeight() * 1.5);
    
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    
    size -= getTextHeight() * 1.5;
    juce::Rectangle<int> r;
    r.setSize  (size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY     (bounds.getY());
    
    return r;
    
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    if( auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param) )
        return choiceParam->getCurrentChoiceName();
    
    juce::String str;
    bool addK = false;
    
    if( auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param) )
    {
        float val = getValue();
        
        addK = truncateKiloValue(val);
        str = juce::String(val, (addK ? 2 : 0));
    }
    else
    {
        jassertfalse; //this shouldn't happen!
    }
    
    if( suffix.isNotEmpty() )
    {
        str << " ";
        if( addK )
            str << "k";
        
        str << suffix;
    }
    
    return str;
}

void RotarySliderWithLabels::changeParam(juce::RangedAudioParameter *p)
{
    param = p;
    repaint();
}
//==============================================================================
juce::String RatioSlider::getDisplayString() const
{
    auto choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param);
    jassert(choiceParam != nullptr);
    
    auto currentChoice = choiceParam->getCurrentChoiceName();
    if( currentChoice.contains(".0") )
        currentChoice = currentChoice.substring(0, currentChoice.indexOf("."));
    
    currentChoice << ":1";
    
    return currentChoice;
}
//==============================================================================
GlobalControls::GlobalControls(juce::AudioProcessorValueTreeState& apvts)
{
    using namespace Params;
    const auto& params = GetParams();
    
    auto getParamHelper = [&params, &apvts](const auto& name) -> auto&
    {
        return getParam(apvts, params, name);
    };
    
    auto& gainInParam =  getParamHelper(Names::Gain_In);
    auto& lowMidParam =  getParamHelper(Names::Low_Mid_Crossover_Freq);
    auto& midHighParam = getParamHelper(Names::Mid_High_Crossover_Freq);
    auto& gainOutParam = getParamHelper(Names::Gain_Out);
    
    inGainSlider =       std::make_unique<RSWL>  (&gainInParam,
                                                  "dB",
                                                  "INPUT TRIM");
    lowMidXoverSlider =  std::make_unique<RSWL>  (&lowMidParam,
                                                  "Hz",
                                                  "LOW-MID X-OVER");
    midHighXoverSlider = std::make_unique<RSWL>  (&midHighParam,
                                                  "Hz",
                                                  "MID-HI X-OVER");
    outGainSlider =      std::make_unique<RSWL>  (&gainOutParam,
                                                  "dB",
                                                  "OUTPUT TRIM");
    
    auto makeAttachmentHelper = [&params, &apvts](auto& attachment,
                                                  const auto& name,
                                                  auto& slider)
    {
        makeAttachment(attachment, apvts, params, name, slider);
    };
    
    makeAttachmentHelper(inGainSliderAttachment,
                         Names::Gain_In,
                         *inGainSlider);
    
    makeAttachmentHelper(lowMidXoverSliderAttachment,
                         Names::Low_Mid_Crossover_Freq,
                         *lowMidXoverSlider);
    
    makeAttachmentHelper(midHighXoverSliderAttachment,
                         Names::Mid_High_Crossover_Freq,
                         *midHighXoverSlider);
    
    makeAttachmentHelper(outGainSliderAttachment,
                         Names::Gain_Out,
                         *outGainSlider);
    
    addLabelPairs(inGainSlider->labels,
                  gainInParam,
                  "dB");
    
    addLabelPairs(lowMidXoverSlider->labels,
                  lowMidParam,
                  "Hz");
    
    addLabelPairs(midHighXoverSlider->labels,
                  midHighParam,
                  "Hz");
    
    addLabelPairs(outGainSlider->labels,
                  gainOutParam,
                  "dB");
    
    addAndMakeVisible(*inGainSlider);
    addAndMakeVisible(*lowMidXoverSlider);
    addAndMakeVisible(*midHighXoverSlider);
    addAndMakeVisible(*outGainSlider);
}

void GlobalControls::paint(juce::Graphics &g)
{
    auto bounds = getLocalBounds();
    drawModuleBackground(g, bounds);
}

void GlobalControls::resized()
{
    auto bounds = getLocalBounds().reduced(5);
    using namespace juce;
    
    FlexBox flexbox;
    flexbox.flexDirection = FlexBox::Direction::row;
    flexbox.flexWrap =      FlexBox::Wrap::noWrap;
    
    auto spacer = FlexItem().withWidth(4);
    auto endCap = FlexItem().withWidth(6);
    
    flexbox.items.add(endCap);
    flexbox.items.add(FlexItem(*inGainSlider).withFlex(1.f));
    flexbox.items.add(spacer);
    flexbox.items.add(FlexItem(*lowMidXoverSlider).withFlex(1.f));
    flexbox.items.add(spacer);
    flexbox.items.add(FlexItem(*midHighXoverSlider).withFlex(1.f));
    flexbox.items.add(spacer);
    flexbox.items.add(FlexItem(*outGainSlider).withFlex(1.f));
    flexbox.items.add(endCap);
    
    flexbox.performLayout(bounds);
}
//==============================================================================
SimpleMBCompAudioProcessorEditor::SimpleMBCompAudioProcessorEditor (SimpleMBCompAudioProcessor& p)
: AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    
    setLookAndFeel(&lnf);
    //    addAndMakeVisible(controlBar);
    addAndMakeVisible(analyzer);
    addAndMakeVisible(globalControls);
    addAndMakeVisible(bandControls);
    
    setSize (600, 500);
}

SimpleMBCompAudioProcessorEditor::~SimpleMBCompAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void SimpleMBCompAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    //
    //    g.setColour (juce::Colours::white);
    //    g.setFont (18.0f);
    //    g.drawFittedText ("Hello from the world of SimpleMBComp!", getLocalBounds(), juce::Justification::centred, 1);
    g.fillAll(juce::Colours::black);
}

void SimpleMBCompAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    
    auto bounds = getLocalBounds();
    
    controlBar.setBounds    ( bounds.removeFromTop(32) );
    bandControls.setBounds  ( bounds.removeFromBottom(135) );
    analyzer.setBounds      ( bounds.removeFromTop(225) );
    globalControls.setBounds( bounds );
}
