#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class PhraserAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PhraserAudioProcessorEditor(PhraserAudioProcessor&);
    ~PhraserAudioProcessorEditor() override = default;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    void configureSlider(juce::Slider&, juce::Label&, const juce::String&);

    PhraserAudioProcessor& processor;
    juce::TextButton loadButton { "Load WAV" };
    juce::TextButton randomizeButton { "Randomize" };
    juce::Label status;
    juce::Slider densitySlider, sliceSlider, gainSlider;
    juce::Label densityLabel, sliceLabel, gainLabel;
    std::unique_ptr<SliderAttachment> densityAttachment, sliceAttachment, gainAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhraserAudioProcessorEditor)
};
