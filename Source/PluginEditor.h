#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

// Custom JUCE-drawn studio interface. All waveform peaks are precalculated.
class PhraserAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit PhraserAudioProcessorEditor(PhraserAudioProcessor&);
    ~PhraserAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    static constexpr int controlCount = 12;
    PhraserAudioProcessor& processor;
    std::unique_ptr<juce::LookAndFeel_V4> theme;
    juce::TextButton generateButton { "GENERATE PHRASE" };
    std::array<juce::TextButton, PhraserAudioProcessor::numSlots> slotButtons;
    std::array<juce::TextButton, 2> saveButtons, recallButtons;
    std::array<juce::Slider, controlCount> sliders;
    std::array<juce::Label, controlCount> sliderLabels;
    std::array<std::unique_ptr<SliderAttachment>, controlCount> attachments;
    std::array<juce::Slider, 4> rangeSliders;
    std::array<juce::Label, 4> rangeLabels;
    juce::Label status;
    std::unique_ptr<juce::FileChooser> chooser;
    int selectedSlot = 0;
    int lastStep = -2;

    void timerCallback() override;
    void updateSlotButtons();
    void updateRanges();
    void applyRanges();
    void loadSlot(int index);
    void paintLanes(juce::Graphics&);
    void paintSequence(juce::Graphics&);
    void paintPanel(juce::Graphics&);
    void updateInfo();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhraserAudioProcessorEditor)
};
