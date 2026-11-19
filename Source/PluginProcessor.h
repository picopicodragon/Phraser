#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <memory>
#include <limits>
#include <vector>

struct PhraserSample
{
    static constexpr int waveformPoints = 160;
    juce::AudioBuffer<float> audio;
    double sampleRate = 44100.0;
    double detectedBpm = 0.0;
    juce::String sourcePath;
    std::vector<float> transients { 0.0f };
    std::array<float, waveformPoints> waveform {};
};

struct SliceEvent
{
    float offset = 0.0f;
    float length = 0.25f;
    int slot = 0;
    int semitones = 0;
    bool enabled = false;
    bool locked = false;
};

struct PhraserPattern
{
    static constexpr int maxSteps = 64;
    std::array<SliceEvent, maxSteps> events {};
};

struct PhraserSlotSettings
{
    float startMin = 0.0f;
    float startMax = 1.0f;
    float lengthMin = 0.08f;
    float lengthMax = 0.35f;
    bool fadeAtNextStep = false;
    bool syncToHost = false;
    double manualBpm = 0.0;
    int pitchSemitones = 0; // Independent sample tuning, -12..+12.
};

// Pattern and sample snapshots are immutable after publication.
// Loading / generation / editing happen outside the audio callback.
class PhraserAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr int numSlots = 16;
    static constexpr int numSteps = PhraserPattern::maxSteps;
    static constexpr int waveformPoints = PhraserSample::waveformPoints;

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
    std::shared_ptr<const PhraserSample> getSampleSnapshot(int slot) const;
    int getTransientCount(int slot) const;
    void randomizePattern();
    void toggleStep(int step);
    void toggleLock(int step);
    void setStepSlot(int step, int slot);
    void saveKeep(int index);
    void recallKeep(int index);
    PhraserPattern getPatternSnapshot() const;
    int getCurrentStep() const { return currentStep.load(std::memory_order_relaxed); }
    int getActiveVoiceCount() const { return activeVoiceCount.load(std::memory_order_relaxed); }
    int getVoiceCountForSlot(int slot) const
    {
        return juce::isPositiveAndBelow(slot, numSlots)
            ? slotVoiceCounts[static_cast<size_t>(slot)].load(std::memory_order_relaxed) : 0;
    }
    double getHostBpm() const { return lastBpm.load(std::memory_order_relaxed); }
    float getPlayPosition(int slot) const;
    int getActiveSteps() const;
    int getStepsPerBar() const;
    PhraserSlotSettings getSlotSettings(int slot) const;
    double getSampleTempo(int slot) const;
    void setSlotSettings(int slot, PhraserSlotSettings);
    juce::AudioProcessorValueTreeState parameters;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();

    std::array<std::shared_ptr<const PhraserSample>, numSlots> samples;
    std::shared_ptr<const PhraserPattern> pattern;
    std::array<std::shared_ptr<const PhraserPattern>, 2> keeps;
    std::array<PhraserSlotSettings, numSlots> settings;
    std::array<std::atomic<bool>, numSlots> fadeAtNextStepModes {};
    std::array<std::atomic<bool>, numSlots> syncToHostModes {};
    std::array<std::atomic<double>, numSlots> manualTempos {};
    std::array<std::atomic<int>, numSlots> slotPitches {};
    mutable juce::CriticalSection settingsLock;
    juce::AudioFormatManager formatManager;
    juce::Random random;
    std::atomic<int> currentStep { -1 };
    std::atomic<int> activeVoiceCount { 0 };
    std::array<std::atomic<int>, numSlots> slotVoiceCounts {};
    std::atomic<double> lastBpm { 120.0 };
    std::array<std::atomic<float>, numSlots> playPositions {};
    double engineSampleRate = 44100.0;
    int64_t previousStep = std::numeric_limits<int64_t>::min();

    struct Voice
    {
        std::shared_ptr<const PhraserSample> source;
        double position = 0.0;
        double end = 0.0;
        double increment = 1.0;
        double rendered = 0.0;
        double duration = 1.0;
        double releaseStart = -1.0;
        double releaseFrames = 1.0;
        int slot = -1;
        bool fadeAtNextStep = false;
        bool syncToHost = false;
        double tempoBpm = 0.0;
        double baseIncrement = 1.0; // Natural source-frame advance per output frame.
        double pitchIncrement = 1.0; // Tuning, independent of timeline advance.
        double sourceStart = 0.0;
        int pitchSemitones = 0;
        static constexpr int grainTimeMs = 30;
        int grainHopFrames = 1;
        int64_t currentGrainHop = -1;
        std::array<double, 2> grainStarts {};
        uint64_t serial = 0;
    };
    static constexpr int maxVoices = 32;
    std::array<Voice, maxVoices> voices {};
    int nextVoice = 0;
    uint64_t voiceSerial = 0;

    void startStep(const SliceEvent&,
                   const std::array<std::shared_ptr<const PhraserSample>, numSlots>&,
                   double releaseFrames);
    void publishVoiceMeters();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhraserAudioProcessor)
};
