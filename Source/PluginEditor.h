#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

class PhraserAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PhraserAudioProcessorEditor(PhraserAudioProcessor&);
    ~PhraserAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    void refreshButtons();
    void loadSlot(int slot);

    PhraserAudioProcessor& processor;
    juce::TextButton randomizeButton { "GENERATE" };
    juce::Label status;
    std::array<juce::TextButton, PhraserAudioProcessor::numSlots> slotButtons;
    std::array<juce::TextButton, PhraserAudioProcessor::numSteps> stepButtons;
    std::array<juce::TextButton, PhraserAudioProcessor::numSteps> lockButtons;
    std::array<juce::Slider, 8> sliders;
    std::array<juce::Label, 8> sliderLabels;
    std::array<std::unique_ptr<SliderAttachment>, 8> attachments;
    std::unique_ptr<juce::FileChooser> chooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhraserAudioProcessorEditor)
};
