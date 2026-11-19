#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>

struct PhraserSample
{
    juce::AudioBuffer<float> audio;
    double sampleRate = 44100.0;
    juce::String sourcePath;
};

struct SliceEvent
{
    float offset = 0.0f;
    float length = 0.25f;
    int slot = 0;
    bool enabled = true;
    bool locked = false;
};

struct PhraserPattern
{
    std::array<SliceEvent, 16> events {};
};

// Immutable snapshots are exchanged between the UI and audio threads.
class PhraserAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr int numSlots = 4;
    static constexpr int numSteps = 16;

    PhraserAudioProcessor();
    ~PhraserAudioProcessor() override = default;

    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Phraser"; }
    bool acceptsMidi() const override { return true; }
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

    bool loadWavFile(int slot, const juce::File&);
    juce::String getSlotName(int slot) const;
    void randomizePattern();
    void toggleStep(int step);
    void toggleLock(int step);
    PhraserPattern getPatternSnapshot() const;
    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();

    std::array<std::shared_ptr<const PhraserSample>, numSlots> samples;
    std::shared_ptr<const PhraserPattern> pattern;
    juce::AudioFormatManager formatManager;
    juce::Random random;
    double engineSampleRate = 44100.0;
    int64_t previousStep = std::numeric_limits<int64_t>::min();

    std::shared_ptr<const PhraserSample> activeSample;
    double voicePosition = 0.0;
    double voiceStart = 0.0;
    double voiceEnd = 0.0;
    double voiceIncrement = 1.0;
    double voiceRendered = 0.0;
    double voiceDuration = 1.0;

    void startStep(const SliceEvent&, const std::array<std::shared_ptr<const PhraserSample>, numSlots>&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhraserAudioProcessor)
};
