#include "PluginEditor.h"

KeyboardAirEditor::KeyboardAirEditor (KeyboardAirProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    bodyAttach   = std::make_unique<Attach> (p.apvts, "bodyAmount",  bodyKnob.slider);
    plateAttach  = std::make_unique<Attach> (p.apvts, "plateAmount", plateKnob.slider);
    decayAttach  = std::make_unique<Attach> (p.apvts, "decay",       decayKnob.slider);
    resonAttach  = std::make_unique<Attach> (p.apvts, "resonance",   resonKnob.slider);
    warmthAttach = std::make_unique<Attach> (p.apvts, "warmth",      warmthKnob.slider);
    mixAttach    = std::make_unique<Attach> (p.apvts, "mix",         mixKnob.slider);

    addAndMakeVisible (bodyKnob);
    addAndMakeVisible (plateKnob);
    addAndMakeVisible (decayKnob);
    addAndMakeVisible (resonKnob);
    addAndMakeVisible (warmthKnob);
    addAndMakeVisible (mixKnob);

    setSize (540, 230);
    setResizable (false, false);
}

KeyboardAirEditor::~KeyboardAirEditor() {}

void KeyboardAirEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient bg (juce::Colour (0xFF14142A), 0.0f, 0.0f,
                              juce::Colour (0xFF0D0D1F), 0.0f, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillRect (getLocalBounds());

    g.setFont (juce::Font ("Arial", 17.0f, juce::Font::bold));
    g.setColour (juce::Colours::white);
    g.drawText ("KEYBOARD AIR", 0, 8, getWidth(), 22, juce::Justification::centred);

    g.setFont (juce::Font ("Arial", 9.0f, juce::Font::italic));
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawText ("Natural Instrument Resonance  —  Yamaha / Roland / Nord style",
                0, 28, getWidth(), 14, juce::Justification::centred);

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (44, 20.0f, (float) getWidth() - 20.0f);

    g.setFont (juce::Font ("Arial", 8.5f, juce::Font::bold));
    g.setColour (juce::Colour (0xFF4FC3D8).withAlpha (0.55f));
    g.drawText ("STAGE 1 - BODY", 12, 48, 160, 13, juce::Justification::left);
    g.setColour (juce::Colour (0xFF60D890).withAlpha (0.55f));
    g.drawText ("STAGE 2 - RESONANCE", 195, 48, 165, 13, juce::Justification::left);
    g.setColour (juce::Colour (0xFFFF7055).withAlpha (0.55f));
    g.drawText ("OUTPUT", 390, 48, 130, 13, juce::Justification::left);

    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRect (181, 48, 1, getHeight() - 56);
    g.fillRect (377, 48, 1, getHeight() - 56);
}

void KeyboardAirEditor::resized()
{
    const int top = 55;
    const int kH  = getHeight() - top - 10;
    const int kW  = 86;

    bodyKnob.setBounds   ( 12, top, kW, kH);
    plateKnob.setBounds  ( 98, top, kW, kH);
    decayKnob.setBounds  (182, top, kW, kH);
    resonKnob.setBounds  (272, top, kW, kH);
    warmthKnob.setBounds (360, top, kW, kH);
    mixKnob.setBounds    (436, top, kW, kH);
}
