#pragma once
#include <JuceHeader.h>

class KeyboardAirProcessor  : public juce::AudioProcessor,
                               public juce::AudioProcessorValueTreeState::Listener
{
public:
    KeyboardAirProcessor();
    ~KeyboardAirProcessor() override;

    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock   (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override              { return true; }

    const juce::String getName() const override;
    bool acceptsMidi()  const override           { return false; }
    bool producesMidi() const override           { return false; }
    bool isMidiEffect() const override           { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int  getNumPrograms()                             override { return 1; }
    int  getCurrentProgram()                          override { return 0; }
    void setCurrentProgram (int)                      override {}
    const juce::String getProgramName (int)           override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void parameterChanged (const juce::String& paramID, float newValue) override;
    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateReverbParams();
    void updateHPFCoefficients (float warmth);

    juce::Reverb             bodyReverb;
    juce::Reverb::Parameters bodyParams;
    juce::Reverb             plateReverb;
    juce::Reverb::Parameters plateParams;

    juce::dsp::IIR::Filter<float> hpfL, hpfR;

    static constexpr int NUM_COMBS = 8;

    struct CombFilter
    {
        juce::AudioBuffer<float> delayBuf { 1, 1 };
        int   writePos     = 0;
        int   delaySamples = 0;
        float dampState    = 0.0f;

        void prepare (int samples)
        {
            delaySamples = samples;
            delayBuf.setSize (1, samples + 2, false, true, false);
            delayBuf.clear();
            writePos  = 0;
            dampState = 0.0f;
        }

        void reset()
        {
            delayBuf.clear();
            writePos  = 0;
            dampState = 0.0f;
        }

        float process (float input, float feedback, float damping) noexcept
        {
            auto* buf     = delayBuf.getWritePointer (0);
            int   size    = delayBuf.getNumSamples();
            int   readPos = writePos - delaySamples;
            if (readPos < 0) readPos += size;
            float output  = buf[readPos];
            dampState     = output * (1.0f - damping) + dampState * damping;
            buf[writePos] = input + dampState * feedback;
            writePos      = (writePos + 1) % size;
            return output;
        }
    };

    CombFilter combsL[NUM_COMBS];
    CombFilter combsR[NUM_COMBS];

    static constexpr float COMB_DELAYS_MS[NUM_COMBS] = {
        9.09f, 6.80f, 4.55f, 3.40f, 2.27f, 1.70f, 1.14f, 0.85f
    };
    static constexpr float COMB_DETUNE_R[NUM_COMBS] = {
        1.021f, 0.983f, 1.015f, 0.979f, 1.019f, 0.981f, 1.013f, 0.987f
    };

    double currentSampleRate = 44100.0;
    juce::AudioBuffer<float> bodyBuffer;
    juce::AudioBuffer<float> plateBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyboardAirProcessor)
};
