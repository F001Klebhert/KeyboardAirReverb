#include "PluginProcessor.h"
#include "PluginEditor.h"

KeyboardAirProcessor::KeyboardAirProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createParameterLayout())
{
    apvts.addParameterListener ("bodyAmount",  this);
    apvts.addParameterListener ("plateAmount", this);
    apvts.addParameterListener ("decay",       this);
    apvts.addParameterListener ("warmth",      this);
}

KeyboardAirProcessor::~KeyboardAirProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout
KeyboardAirProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "bodyAmount", "Body",      0.0f, 1.0f, 0.55f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "plateAmount","Plate",     0.0f, 1.0f, 0.35f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "decay",      "Decay",     0.0f, 1.0f, 0.40f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "resonance",  "Resonance", 0.0f, 1.0f, 0.30f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "warmth",     "Warmth",    0.0f, 1.0f, 0.60f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "mix",        "Mix",       0.0f, 1.0f, 0.75f));
    return { params.begin(), params.end() };
}

void KeyboardAirProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    bodyReverb.setSampleRate  (sampleRate); bodyReverb.reset();
    plateReverb.setSampleRate (sampleRate); plateReverb.reset();
    updateReverbParams();

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 1;
    hpfL.prepare (spec); hpfL.reset();
    hpfR.prepare (spec); hpfR.reset();
    updateHPFCoefficients (*apvts.getRawParameterValue ("warmth"));

    for (int i = 0; i < NUM_COMBS; ++i)
    {
        int sL = juce::jmax (1, juce::roundToInt (
            COMB_DELAYS_MS[i] * 0.001f * (float) sampleRate));
        int sR = juce::jmax (1, juce::roundToInt (
            COMB_DELAYS_MS[i] * 0.001f * (float) sampleRate * COMB_DETUNE_R[i]));
        combsL[i].prepare (sL);
        combsR[i].prepare (sR);
    }

    bodyBuffer.setSize  (2, samplesPerBlock, false, true, false);
    plateBuffer.setSize (2, samplesPerBlock, false, true, false);
}

void KeyboardAirProcessor::releaseResources()
{
    bodyReverb.reset(); plateReverb.reset();
    hpfL.reset(); hpfR.reset();
    for (int i = 0; i < NUM_COMBS; ++i) { combsL[i].reset(); combsR[i].reset(); }
}

bool KeyboardAirProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}

void KeyboardAirProcessor::updateReverbParams()
{
    const float body  = *apvts.getRawParameterValue ("bodyAmount");
    const float plate = *apvts.getRawParameterValue ("plateAmount");
    const float decay = *apvts.getRawParameterValue ("decay");

    bodyParams.roomSize   = 0.05f + decay * 0.25f;
    bodyParams.damping    = 0.85f;
    bodyParams.wetLevel   = body * 0.45f;
    bodyParams.dryLevel   = 0.0f;
    bodyParams.width      = 0.65f;
    bodyParams.freezeMode = 0.0f;
    bodyReverb.setParameters (bodyParams);

    plateParams.roomSize   = 0.20f + decay * 0.60f;
    plateParams.damping    = 0.30f;
    plateParams.wetLevel   = plate * 0.30f;
    plateParams.dryLevel   = 0.0f;
    plateParams.width      = 1.0f;
    plateParams.freezeMode = 0.0f;
    plateReverb.setParameters (plateParams);
}

void KeyboardAirProcessor::updateHPFCoefficients (float warmth)
{
    float cutoff = juce::jlimit (20.0f, 18000.0f,
                    juce::jmap (warmth, 0.0f, 1.0f, 600.0f, 80.0f));
    auto c = juce::dsp::IIR::Coefficients<float>::makeHighPass (
                 currentSampleRate, cutoff, 0.707f);
    *hpfL.coefficients = *c;
    *hpfR.coefficients = *c;
}

void KeyboardAirProcessor::parameterChanged (const juce::String& id, float val)
{
    if (id == "bodyAmount" || id == "plateAmount" || id == "decay")
        updateReverbParams();
    else if (id == "warmth")
        updateHPFCoefficients (val);
}

void KeyboardAirProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numIn      = getTotalNumInputChannels();
    const int numOut     = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, numSamples);
    if (numIn == 0 || numSamples == 0) return;

    const float resonance = *apvts.getRawParameterValue ("resonance");
    const float mix       = *apvts.getRawParameterValue ("mix");
    const int   chR       = (numIn > 1) ? 1 : 0;

    bodyBuffer.copyFrom (0, 0, buffer, 0,   0, numSamples);
    bodyBuffer.copyFrom (1, 0, buffer, chR, 0, numSamples);

    if (resonance > 0.001f)
    {
        const float fb      = 0.08f + resonance * 0.32f;
        const float damp    = 0.55f;
        const float gain    = resonance * 0.09f / (float) NUM_COMBS;
        float* bL = bodyBuffer.getWritePointer (0);
        float* bR = bodyBuffer.getWritePointer (1);
        for (int n = 0; n < numSamples; ++n)
        {
            float rL = 0.0f, rR = 0.0f;
            for (int c = 0; c < NUM_COMBS; ++c)
            {
                rL += combsL[c].process (bL[n], fb, damp);
                rR += combsR[c].process (bR[n], fb, damp);
            }
            bL[n] += rL * gain;
            bR[n] += rR * gain;
        }
    }

    plateBuffer.copyFrom (0, 0, bodyBuffer, 0, 0, numSamples);
    plateBuffer.copyFrom (1, 0, bodyBuffer, 1, 0, numSamples);

    bodyReverb.processStereo (bodyBuffer.getWritePointer(0),
                               bodyBuffer.getWritePointer(1), numSamples);
    plateReverb.processStereo(plateBuffer.getWritePointer(0),
                               plateBuffer.getWritePointer(1), numSamples);

    {
        float* pL = plateBuffer.getWritePointer (0);
        float* pR = plateBuffer.getWritePointer (1);
        juce::dsp::AudioBlock<float> bkL (&pL, 1, (size_t) numSamples);
        juce::dsp::AudioBlock<float> bkR (&pR, 1, (size_t) numSamples);
        hpfL.process (juce::dsp::ProcessContextReplacing<float> (bkL));
        hpfR.process (juce::dsp::ProcessContextReplacing<float> (bkR));
    }

    for (int ch = 0; ch < juce::jmin (2, numOut); ++ch)
    {
        float*       out   = buffer.getWritePointer (ch);
        const float* body  = bodyBuffer.getReadPointer  (ch);
        const float* plate = plateBuffer.getReadPointer (ch);
        for (int n = 0; n < numSamples; ++n)
            out[n] += (body[n] + plate[n]) * mix;
    }
}

void KeyboardAirProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void KeyboardAirProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

const juce::String KeyboardAirProcessor::getName() const { return JucePlugin_Name; }

juce::AudioProcessorEditor* KeyboardAirProcessor::createEditor()
{
    return new KeyboardAirEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KeyboardAirProcessor();
}
