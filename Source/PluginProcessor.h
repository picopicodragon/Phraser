#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>

struct PhraserSample
{
    juce::AudioBuffer<float> audio;
    double sampleRate = 44100.0;
};

struct SliceEvent
{
    float offset = 0.0f;
    float length = 0.12f;
    bool enabled = false;
};

struct PhraserPattern
{
    std::array<SliceEvent, 16> events {};
};

class PhraserAudioProcessor final : public juce::AudioProcessor
{
public:
    PhraserAudioProcessor();
    ~PhraserAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Phraser"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    bool loadWavFile(const juce::File&);
    void randomizePattern();
    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    std::shared_ptr<const PhraserSample> sample;
    std::shared_ptr<const PhraserPattern> pattern;
    juce::AudioFormatManager formatManager;
    juce::Random random;
    double engineSampleRate = 44100.0;
    int64_t previousStep = -1;
    double voicePosition = 0.0;
    double voiceEnd = 0.0;
    double voiceIncrement = 1.0;
    std::shared_ptr<const PhraserSample> activeSample;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhraserAudioProcessor)
};
