#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class KnobComponent : public juce::Component
{
public:
    juce::Slider slider { juce::Slider::RotaryVerticalDrag,
                          juce::Slider::TextBoxBelow };
    juce::Label  label;

    KnobComponent (const juce::String& name, juce::Colour accent)
    {
        slider.setColour (juce::Slider::rotarySliderFillColourId,    accent);
        slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xFF2A2A40));
        slider.setColour (juce::Slider::thumbColourId,               juce::Colours::white);
        slider.setColour (juce::Slider::textBoxTextColourId,         juce::Colours::white.withAlpha (0.65f));
        slider.setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
        addAndMakeVisible (slider);

        label.setText (name, juce::dontSendNotification);
        label.setFont (juce::Font ("Arial", 9.5f, juce::Font::bold));
        label.setColour (juce::Label::textColourId, accent.withAlpha (0.85f));
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        label.setBounds (b.removeFromBottom (17));
        slider.setBounds (b);
    }
};

class KeyboardAirEditor : public juce::AudioProcessorEditor
{
public:
    explicit KeyboardAirEditor (KeyboardAirProcessor&);
    ~KeyboardAirEditor() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

private:
    KeyboardAirProcessor& processor;

    KnobComponent bodyKnob   { "BODY",      juce::Colour (0xFF4FC3D8) };
    KnobComponent plateKnob  { "PLATE",     juce::Colour (0xFF4FC3D8) };
    KnobComponent decayKnob  { "DECAY",     juce::Colour (0xFFF0C040) };
    KnobComponent resonKnob  { "RESONANCE", juce::Colour (0xFF60D890) };
    KnobComponent warmthKnob { "WARMTH",    juce::Colour (0xFFF0C040) };
    KnobComponent mixKnob    { "MIX",       juce::Colour (0xFFFF7055) };

    using Attach = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attach> bodyAttach, plateAttach, decayAttach,
                             resonAttach, warmthAttach, mixAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyboardAirEditor)
};
