#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

class PhraserAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit PhraserAudioProcessorEditor(PhraserAudioProcessor&);
    ~PhraserAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    static constexpr int controlCount = 12;
    static constexpr float waveLeft = 427.0f;
    static constexpr float waveWidth = 431.0f;
    static constexpr float laneTop = 102.0f;
    static constexpr float lanePitch = 30.0f;

    PhraserAudioProcessor& processor;
    std::unique_ptr<juce::LookAndFeel_V4> theme;
    juce::TextButton generateButton { "GENERATE" };
    std::array<juce::TextButton, PhraserAudioProcessor::numSlots> slotButtons;
    std::array<juce::TextButton, PhraserAudioProcessor::numSlots> modeButtons;
    std::array<juce::TextButton, PhraserAudioProcessor::numSlots> syncButtons;
    std::array<juce::Label, PhraserAudioProcessor::numSlots> tempoLabels;
    std::array<juce::Label, PhraserAudioProcessor::numSlots> pitchLabels;
    std::array<juce::TextButton, 2> saveButtons, recallButtons;
    std::array<juce::Slider, controlCount> sliders;
    std::array<juce::Label, controlCount> sliderLabels;
    std::array<std::unique_ptr<SliderAttachment>, controlCount> attachments;
    juce::Label status;
    std::unique_ptr<juce::FileChooser> chooser;
    int selectedSlot = 0;
    int dragSlot = -1;
    float dragAnchor = 0.0f;
    bool dragDuration = false;
    int lastStep = -2;

    void timerCallback() override;
    void updateSlotControls();
    void loadSlot(int index);
    void paintLanes(juce::Graphics&);
    void paintSequence(juce::Graphics&);
    void paintOverlapOrbit(juce::Graphics&);
    void applyWaveDrag(const juce::MouseEvent&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhraserAudioProcessorEditor)
};
